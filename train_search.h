#ifndef TRAIN_SEARCH_H
#define TRAIN_SEARCH_H

#include <stddef.h>

/*
 * Searches a read-only snapshot of train and schedule data using pthreads.
 * Set journeyDate to NULL for route-only search. When journeyDate is non-NULL,
 * a matching non-cancelled schedule for that date is required.
 * Returns number of matching train numbers, or -1 on allocation/limit error.
 */
int threadedTrainSearch(const char *source,
                        const char *destination,
                        const char *journeyDate,
                        int *trainNumbers,
                        size_t resultCapacity);

#endif
