#include "train_search.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "train.h"
#include "trainschedule.h"

#define SEARCH_THREAD_COUNT 4U

typedef struct
{
    Train *trains;
    size_t trainCount;
    TrainSchedule *schedules;
    size_t scheduleCount;
    const char *source;
    const char *destination;
    const char *journeyDate;
    unsigned char *matches;
    size_t begin;
    size_t end;
} SearchWorker;

static int stationIndexInSnapshot(const Train *train, const char *name)
{
    int i;

    for (i = 0; i < train->totalStations; ++i)
    {
        if (strcmp(train->stations[i].stationName, name) == 0)
        {
            return i;
        }
    }

    return -1;
}

static int hasSchedule(const SearchWorker *worker, int trainNumber)
{
    size_t i;

    for (i = 0U; i < worker->scheduleCount; ++i)
    {
        if ((worker->schedules[i].trainNumber == trainNumber) &&
            (strcmp(worker->schedules[i].journeyDate,
                    worker->journeyDate) == 0))
        {
            return worker->schedules[i].status != TRAIN_CANCELLED;
        }
    }

    return 0;
}

static void *searchWorker(void *argument)
{
    SearchWorker *worker = (SearchWorker *)argument;
    size_t i;

    for (i = worker->begin; i < worker->end; ++i)
    {
        const Train *train = &worker->trains[i];
        int sourceIndex = stationIndexInSnapshot(train, worker->source);
        int destinationIndex = stationIndexInSnapshot(train,
                                                       worker->destination);

        if ((sourceIndex >= 0) && (destinationIndex > sourceIndex))
        {
            if ((worker->journeyDate == NULL) ||
                hasSchedule(worker, train->trainNumber))
            {
                worker->matches[i] = 1U;
            }
        }
    }

    return NULL;
}

static void freeSnapshots(Train *trains,
                          TrainSchedule *schedules,
                          unsigned char *matches)
{
    free(trains);
    free(schedules);
    free(matches);
}

int threadedTrainSearch(const char *source,
                        const char *destination,
                        const char *journeyDate,
                        int *trainNumbers,
                        size_t resultCapacity)
{
    Train *trainSnapshot = NULL;
    TrainSchedule *scheduleSnapshot = NULL;
    unsigned char *matches = NULL;
    const Train *trainNode;
    const TrainSchedule *scheduleNode;
    size_t trainCount = 0U;
    size_t scheduleCount = 0U;
    size_t i;
    size_t copied;
    size_t resultCount = 0U;
    SearchWorker workers[SEARCH_THREAD_COUNT];
    pthread_t threads[SEARCH_THREAD_COUNT];
    unsigned char started[SEARCH_THREAD_COUNT] = {0U, 0U, 0U, 0U};

    if ((source == NULL) || (destination == NULL) ||
        (trainNumbers == NULL) || (resultCapacity == 0U))
    {
        return -1;
    }

    /* Count and copy all shared data before workers are started. */
    for (trainNode = trainHead; trainNode != NULL; trainNode = trainNode->next)
    {
        ++trainCount;
    }
    for (scheduleNode = scheduleHead; scheduleNode != NULL;
         scheduleNode = scheduleNode->next)
    {
        ++scheduleCount;
    }

    if (trainCount == 0U)
    {
        return 0;
    }
    if (trainCount > resultCapacity)
    {
        return -1;
    }

    trainSnapshot = (Train *)calloc(trainCount, sizeof *trainSnapshot);
    matches = (unsigned char *)calloc(trainCount, sizeof *matches);
    if ((journeyDate != NULL) && (scheduleCount > 0U))
    {
        scheduleSnapshot = (TrainSchedule *)calloc(scheduleCount,
                                                    sizeof *scheduleSnapshot);
    }
    if ((trainSnapshot == NULL) || (matches == NULL) ||
        ((journeyDate != NULL) && (scheduleCount > 0U) &&
         (scheduleSnapshot == NULL)))
    {
        freeSnapshots(trainSnapshot, scheduleSnapshot, matches);
        return -1;
    }

    trainNode = trainHead;
    for (i = 0U; i < trainCount; ++i)
    {
        trainSnapshot[i] = *trainNode;
        trainSnapshot[i].prev = NULL;
        trainSnapshot[i].next = NULL;
        trainNode = trainNode->next;
    }

    if (scheduleSnapshot != NULL)
    {
        scheduleNode = scheduleHead;
        for (i = 0U; i < scheduleCount; ++i)
        {
            scheduleSnapshot[i] = *scheduleNode;
            scheduleSnapshot[i].prev = NULL;
            scheduleSnapshot[i].next = NULL;
            scheduleNode = scheduleNode->next;
        }
    }

    copied = 0U;
    for (i = 0U; i < SEARCH_THREAD_COUNT; ++i)
    {
        size_t remaining = trainCount - copied;
        size_t chunk = remaining / (SEARCH_THREAD_COUNT - i);

        workers[i].trains = trainSnapshot;
        workers[i].trainCount = trainCount;
        workers[i].schedules = scheduleSnapshot;
        workers[i].scheduleCount = scheduleSnapshot != NULL ? scheduleCount : 0U;
        workers[i].source = source;
        workers[i].destination = destination;
        workers[i].journeyDate = journeyDate;
        workers[i].matches = matches;
        workers[i].begin = copied;
        workers[i].end = copied + chunk;
        copied += chunk;

        if (workers[i].begin < workers[i].end)
        {
            if (pthread_create(&threads[i], NULL, searchWorker, &workers[i]) == 0)
            {
                started[i] = 1U;
            }
            else
            {
                /* Fallback: process this chunk synchronously if creation fails. */
                (void)searchWorker(&workers[i]);
            }
        }
    }

    for (i = 0U; i < SEARCH_THREAD_COUNT; ++i)
    {
        if (started[i] != 0U)
        {
            (void)pthread_join(threads[i], NULL);
        }
    }

    for (i = 0U; i < trainCount; ++i)
    {
        if (matches[i] != 0U)
        {
            trainNumbers[resultCount] = trainSnapshot[i].trainNumber;
            ++resultCount;
        }
    }

    freeSnapshots(trainSnapshot, scheduleSnapshot, matches);
    return (int)resultCount;
}
