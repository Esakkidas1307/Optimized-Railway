#define _POSIX_C_SOURCE 200809L
#include "optimization_runtime.h"

#include <errno.h>
#include <time.h>
#include <string.h>
#include <signal.h>

static pthread_mutex_t runtime_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t runtime_condition = PTHREAD_COND_INITIALIZER;
static pthread_t saver_thread;
static int runtime_started;
static int runtime_stop;
static int save_requested;
static int timeout_requested;
static volatile sig_atomic_t interrupt_requested;
static unsigned save_interval = 30U;
static unsigned char dirty[RAILWAY_DATA_FILE_COUNT];
static RailwayRuntimeSaveCallback savers[RAILWAY_DATA_FILE_COUNT];

static void railway_signal_handler(int signal_number)
{
    (void)signal_number;
    interrupt_requested = 1;
}

static void *saver_worker(void *unused)
{
    (void)unused;
    (void)pthread_mutex_lock(&runtime_mutex);
    while (runtime_stop == 0) {
        struct timespec deadline;
        (void)clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += (time_t)save_interval;
        int rc = 0;
        while ((rc != ETIMEDOUT) && (runtime_stop == 0)) {
            rc = pthread_cond_timedwait(&runtime_condition, &runtime_mutex, &deadline);
            if (rc != 0 && rc != ETIMEDOUT && rc != EINTR) {
                break;
            }
        }
        if (runtime_stop == 0) {
            save_requested = 1;
        }
    }
    (void)pthread_mutex_unlock(&runtime_mutex);
    return NULL;
}

int railway_runtime_init(unsigned interval_seconds)
{
    int rc;
    (void)pthread_mutex_lock(&runtime_mutex);
    (void)memset(dirty, 0, sizeof(dirty));
    (void)memset(savers, 0, sizeof(savers));
    runtime_stop = 0;
    save_requested = 0;
    timeout_requested = 0;
    interrupt_requested = 0;
    save_interval = interval_seconds == 0U ? 30U : interval_seconds;
    (void)pthread_mutex_unlock(&runtime_mutex);
    (void)signal(SIGINT, railway_signal_handler);
    (void)signal(SIGTERM, railway_signal_handler);
    rc = pthread_create(&saver_thread, NULL, saver_worker, NULL);
    if (rc != 0) {
        return rc;
    }
    runtime_started = 1;
    return 0;
}

int railway_runtime_register_saver(RailwayDataFile file, RailwayRuntimeSaveCallback callback)
{
    if ((file < RAILWAY_DATA_TRAINS) || (file >= RAILWAY_DATA_FILE_COUNT) || callback == NULL) {
        return -1;
    }
    (void)pthread_mutex_lock(&runtime_mutex);
    savers[file] = callback;
    (void)pthread_mutex_unlock(&runtime_mutex);
    return 0;
}

void railway_runtime_mark_dirty(RailwayDataFile file)
{
    if ((file < RAILWAY_DATA_TRAINS) || (file >= RAILWAY_DATA_FILE_COUNT)) {
        return;
    }
    (void)pthread_mutex_lock(&runtime_mutex);
    dirty[file] = 1U;
    save_requested = 1;
    (void)pthread_cond_signal(&runtime_condition);
    (void)pthread_mutex_unlock(&runtime_mutex);
}

void railway_runtime_mark_clean(RailwayDataFile file)
{
    if ((file < RAILWAY_DATA_TRAINS) || (file >= RAILWAY_DATA_FILE_COUNT)) {
        return;
    }
    (void)pthread_mutex_lock(&runtime_mutex);
    dirty[file] = 0U;
    (void)pthread_mutex_unlock(&runtime_mutex);
}

int railway_runtime_is_dirty(RailwayDataFile file)
{
    int result = 0;
    if ((file < RAILWAY_DATA_TRAINS) || (file >= RAILWAY_DATA_FILE_COUNT)) {
        return 0;
    }
    (void)pthread_mutex_lock(&runtime_mutex);
    result = dirty[file] != 0U;
    (void)pthread_mutex_unlock(&runtime_mutex);
    return result;
}

void railway_runtime_service(void)
{
    RailwayRuntimeSaveCallback callbacks[RAILWAY_DATA_FILE_COUNT];
    unsigned char pending[RAILWAY_DATA_FILE_COUNT];
    size_t i;

    (void)pthread_mutex_lock(&runtime_mutex);
    if (save_requested == 0) {
        (void)pthread_mutex_unlock(&runtime_mutex);
        return;
    }
    save_requested = 0;
    (void)memcpy(callbacks, savers, sizeof(callbacks));
    (void)memcpy(pending, dirty, sizeof(pending));
    (void)pthread_mutex_unlock(&runtime_mutex);

    /* This function is called by the main thread at safe input-wait points. */
    for (i = 0U; i < (size_t)RAILWAY_DATA_FILE_COUNT; ++i) {
        if ((pending[i] != 0U) && (callbacks[i] != NULL) && (callbacks[i]() == 0)) {
            railway_runtime_mark_clean((RailwayDataFile)i);
        }
    }
}

void railway_runtime_flush(void)
{
    size_t i;
    for (i = 0U; i < (size_t)RAILWAY_DATA_FILE_COUNT; ++i) {
        RailwayRuntimeSaveCallback callback;
        int pending;
        (void)pthread_mutex_lock(&runtime_mutex);
        pending = dirty[i] != 0U;
        callback = savers[i];
        (void)pthread_mutex_unlock(&runtime_mutex);
        if (pending && callback != NULL && callback() == 0) {
            railway_runtime_mark_clean((RailwayDataFile)i);
        }
    }
}

void railway_runtime_shutdown(void)
{
    if (runtime_started != 0) {
        (void)pthread_mutex_lock(&runtime_mutex);
        runtime_stop = 1;
        (void)pthread_cond_signal(&runtime_condition);
        (void)pthread_mutex_unlock(&runtime_mutex);
        (void)pthread_join(saver_thread, NULL);
        runtime_started = 0;
    }
    railway_runtime_flush();
}

void railway_runtime_request_timeout(void)
{
    (void)pthread_mutex_lock(&runtime_mutex);
    timeout_requested = 1;
    (void)pthread_mutex_unlock(&runtime_mutex);
}

int railway_runtime_timeout_requested(void)
{
    int result;
    (void)pthread_mutex_lock(&runtime_mutex);
    result = timeout_requested;
    (void)pthread_mutex_unlock(&runtime_mutex);
    return result;
}

int railway_runtime_interruption_requested(void)
{
    return interrupt_requested != 0 ? 1 : 0;
}
