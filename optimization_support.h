#ifndef OPTIMIZATION_SUPPORT_H
#define OPTIMIZATION_SUPPORT_H

#include <pthread.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RAILWAY_INPUT_TIMEOUT 0
#define RAILWAY_INPUT_OK      1
#define RAILWAY_INPUT_ERROR (-1)

/* One dirty bit per persistent collection. */
typedef enum RailwayDataFile {
    RAILWAY_DATA_TRAINS = 0,
    RAILWAY_DATA_SCHEDULES,
    RAILWAY_DATA_PASSENGERS,
    RAILWAY_DATA_RUNNING,
    RAILWAY_DATA_USERS,
    RAILWAY_DATA_BOOKINGS,
    RAILWAY_DATA_FILE_COUNT
} RailwayDataFile;

typedef int (*RailwaySaveCallback)(void *context);

/* Initialize once before starting background saving; destroy after joining it. */
int railway_persistence_init(void);
void railway_persistence_destroy(void);
void railway_mark_dirty(RailwayDataFile file);
int railway_is_dirty(RailwayDataFile file);

/*
 * Save one dirty collection while holding the persistence mutex.
 * The callback must write the collection and return 0 only on success.
 */
int railway_save_if_dirty(RailwayDataFile file,
                          RailwaySaveCallback save_callback,
                          void *context);

/* Wait for an input line with an inactivity timeout (POSIX terminal/stdin). */
int railway_read_line_timeout(const char *prompt,
                              char *buffer,
                              size_t buffer_size,
                              unsigned timeout_seconds);

/* Generic sorted pointer index. comparator(a,b) returns <0, 0, >0. */
typedef int (*RailwayPointerComparator)(const void *a, const void *b);
typedef struct RailwayPointerIndex {
    void **items;
    size_t count;
    size_t capacity;
} RailwayPointerIndex;

void railway_pointer_index_init(RailwayPointerIndex *index);
void railway_pointer_index_destroy(RailwayPointerIndex *index);
int railway_pointer_index_build(RailwayPointerIndex *index,
                                void *const *items,
                                size_t count,
                                RailwayPointerComparator comparator);
void *railway_pointer_index_find(const RailwayPointerIndex *index,
                                 const void *key,
                                 RailwayPointerComparator comparator);
size_t railway_pointer_index_lower_bound(const RailwayPointerIndex *index,
                                         const void *key,
                                         RailwayPointerComparator comparator);

#ifdef __cplusplus
}
#endif
#endif
