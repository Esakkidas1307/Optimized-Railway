#define _POSIX_C_SOURCE 200809L
#include "optimization_support.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t persistence_mutex;
static int persistence_initialized;
static int dirty_flags[RAILWAY_DATA_FILE_COUNT];

int railway_persistence_init(void)
{
    int rc = pthread_mutex_init(&persistence_mutex, NULL);
    if (rc != 0) {
        return -1;
    }
    (void)memset(dirty_flags, 0, sizeof(dirty_flags));
    persistence_initialized = 1;
    return 0;
}

void railway_persistence_destroy(void)
{
    if (persistence_initialized != 0) {
        (void)pthread_mutex_destroy(&persistence_mutex);
        persistence_initialized = 0;
    }
}

void railway_mark_dirty(RailwayDataFile file)
{
    if ((persistence_initialized != 0) &&
        (file >= RAILWAY_DATA_TRAINS) &&
        (file < RAILWAY_DATA_FILE_COUNT)) {
        (void)pthread_mutex_lock(&persistence_mutex);
        dirty_flags[file] = 1;
        (void)pthread_mutex_unlock(&persistence_mutex);
    }
}

int railway_is_dirty(RailwayDataFile file)
{
    int dirty = 0;
    if ((persistence_initialized != 0) &&
        (file >= RAILWAY_DATA_TRAINS) &&
        (file < RAILWAY_DATA_FILE_COUNT)) {
        (void)pthread_mutex_lock(&persistence_mutex);
        dirty = dirty_flags[file];
        (void)pthread_mutex_unlock(&persistence_mutex);
    }
    return dirty;
}

int railway_save_if_dirty(RailwayDataFile file,
                          RailwaySaveCallback save_callback,
                          void *context)
{
    int result;
    if ((persistence_initialized == 0) ||
        (file < RAILWAY_DATA_TRAINS) ||
        (file >= RAILWAY_DATA_FILE_COUNT) ||
        (save_callback == NULL)) {
        return -1;
    }

    /*
     * This simple API serializes the save callback and dirty flag. Callbacks
     * must not call railway_mark_dirty() or railway_is_dirty() because the
     * mutex is held. Application integration should lock the relevant DLL
     * consistently around mutations and snapshots as well.
     */
    (void)pthread_mutex_lock(&persistence_mutex);
    if (dirty_flags[file] == 0) {
        (void)pthread_mutex_unlock(&persistence_mutex);
        return 0;
    }
    result = save_callback(context);
    if (result == 0) {
        dirty_flags[file] = 0;
    }
    (void)pthread_mutex_unlock(&persistence_mutex);
    return result;
}

int railway_read_line_timeout(const char *prompt,
                              char *buffer,
                              size_t buffer_size,
                              unsigned timeout_seconds)
{
    struct pollfd descriptor;
    int poll_result;
    size_t length;

    if ((buffer == NULL) || (buffer_size < 2U)) {
        return RAILWAY_INPUT_ERROR;
    }

    if (prompt != NULL) {
        (void)fputs(prompt, stdout);
        (void)fflush(stdout);
    }

    descriptor.fd = STDIN_FILENO;
    descriptor.events = POLLIN;
    descriptor.revents = 0;

    do {
        unsigned long milliseconds =
            (unsigned long)timeout_seconds * 1000UL;
        int timeout_ms = (milliseconds > (unsigned long)INT_MAX)
                       ? INT_MAX : (int)milliseconds;
        poll_result = poll(&descriptor, 1U, timeout_ms);
    } while ((poll_result < 0) && (errno == EINTR));

    if (poll_result == 0) {
        buffer[0] = '\0';
        return RAILWAY_INPUT_TIMEOUT;
    }
    if (poll_result < 0) {
        buffer[0] = '\0';
        return RAILWAY_INPUT_ERROR;
    }

    if (fgets(buffer, (int)buffer_size, stdin) == NULL) {
        buffer[0] = '\0';
        return RAILWAY_INPUT_ERROR;
    }

    length = strlen(buffer);
    if ((length > 0U) && (buffer[length - 1U] == '\n')) {
        buffer[length - 1U] = '\0';
        return RAILWAY_INPUT_OK;
    }

    if (!feof(stdin)) {
        int character;
        do {
            character = getchar();
        } while ((character != '\n') && (character != EOF));
    }
    return RAILWAY_INPUT_OK;
}

static void merge_pointers(void **items,
                           void **scratch,
                           size_t left,
                           size_t middle,
                           size_t right,
                           RailwayPointerComparator comparator)
{
    size_t i = left;
    size_t j = middle;
    size_t k = left;

    while ((i < middle) && (j < right)) {
        if (comparator(items[i], items[j]) <= 0) {
            scratch[k++] = items[i++];
        } else {
            scratch[k++] = items[j++];
        }
    }
    while (i < middle) {
        scratch[k++] = items[i++];
    }
    while (j < right) {
        scratch[k++] = items[j++];
    }
    for (i = left; i < right; ++i) {
        items[i] = scratch[i];
    }
}

static void merge_sort_pointers(void **items,
                                void **scratch,
                                size_t left,
                                size_t right,
                                RailwayPointerComparator comparator)
{
    size_t middle;
    if ((right - left) < 2U) {
        return;
    }
    middle = left + ((right - left) / 2U);
    merge_sort_pointers(items, scratch, left, middle, comparator);
    merge_sort_pointers(items, scratch, middle, right, comparator);
    merge_pointers(items, scratch, left, middle, right, comparator);
}

void railway_pointer_index_init(RailwayPointerIndex *index)
{
    if (index != NULL) {
        index->items = NULL;
        index->count = 0U;
        index->capacity = 0U;
    }
}

void railway_pointer_index_destroy(RailwayPointerIndex *index)
{
    if (index != NULL) {
        free(index->items);
        index->items = NULL;
        index->count = 0U;
        index->capacity = 0U;
    }
}

int railway_pointer_index_build(RailwayPointerIndex *index,
                                void *const *items,
                                size_t count,
                                RailwayPointerComparator comparator)
{
    void **copy;
    void **scratch;
    if ((index == NULL) || (comparator == NULL) ||
        ((count > 0U) && (items == NULL))) {
        return -1;
    }

    copy = NULL;
    scratch = NULL;
    if (count > 0U) {
        if (count > (SIZE_MAX / sizeof(*copy))) {
            return -1;
        }
        copy = malloc(count * sizeof(*copy));
        scratch = malloc(count * sizeof(*scratch));
        if ((copy == NULL) || (scratch == NULL)) {
            free(copy);
            free(scratch);
            return -1;
        }
        (void)memcpy(copy, items, count * sizeof(*copy));
        merge_sort_pointers(copy, scratch, 0U, count, comparator);
        free(scratch);
    }

    free(index->items);
    index->items = copy;
    index->count = count;
    index->capacity = count;
    return 0;
}

size_t railway_pointer_index_lower_bound(const RailwayPointerIndex *index,
                                         const void *key,
                                         RailwayPointerComparator comparator)
{
    size_t low = 0U;
    size_t high;
    if ((index == NULL) || (comparator == NULL)) {
        return 0U;
    }
    high = index->count;
    while (low < high) {
        size_t middle = low + ((high - low) / 2U);
        if (comparator(index->items[middle], key) < 0) {
            low = middle + 1U;
        } else {
            high = middle;
        }
    }
    return low;
}

void *railway_pointer_index_find(const RailwayPointerIndex *index,
                                 const void *key,
                                 RailwayPointerComparator comparator)
{
    size_t position;
    if ((index == NULL) || (comparator == NULL) ||
        (index->count == 0U)) {
        return NULL;
    }
    position = railway_pointer_index_lower_bound(index, key, comparator);
    if ((position < index->count) &&
        (comparator(index->items[position], key) == 0)) {
        return index->items[position];
    }
    return NULL;
}
