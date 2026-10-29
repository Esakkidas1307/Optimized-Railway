#include <gtest/gtest.h>

extern "C" {
#include "train_search.h"
#include "train.h"
#include "trainschedule.h"
}

#include <cstring>
#include <string>

namespace {

void clearTrainList()
{
    Train *node = trainHead;
    while (node != nullptr) {
        Train *next = node->next;
        free(node);
        node = next;
    }
    trainHead = nullptr;
}

void clearScheduleList()
{
    TrainSchedule *node = scheduleHead;
    while (node != nullptr) {
        TrainSchedule *next = node->next;
        free(node);
        node = next;
    }
    scheduleHead = nullptr;
}

Train *makeTrain(int number,
                 const char *source,
                 const char *destination,
                 int sourceDistance = 0,
                 int destinationDistance = 350)
{
    Train *train = static_cast<Train *>(calloc(1, sizeof(Train)));
    EXPECT_NE(train, nullptr);
    if (train == nullptr) {
        return nullptr;
    }

    train->trainNumber = number;
    train->totalStations = 2;
    std::strncpy(train->stations[0].stationName,
                 source,
                 sizeof(train->stations[0].stationName) - 1);
    std::strncpy(train->stations[1].stationName,
                 destination,
                 sizeof(train->stations[1].stationName) - 1);
    train->stations[0].distanceFromSource = sourceDistance;
    train->stations[1].distanceFromSource = destinationDistance;

    // Add to the global doubly-linked list.
    train->prev = nullptr;
    train->next = trainHead;
    if (trainHead != nullptr) {
        trainHead->prev = train;
    }
    trainHead = train;
    return train;
}

TrainSchedule *makeSchedule(int number,
                            const char *date,
                            int status = TRAIN_RUNNING)
{
    TrainSchedule *schedule =
        static_cast<TrainSchedule *>(calloc(1, sizeof(TrainSchedule)));
    EXPECT_NE(schedule, nullptr);
    if (schedule == nullptr) {
        return nullptr;
    }

    schedule->trainNumber = number;
    std::strncpy(schedule->journeyDate,
                 date,
                 sizeof(schedule->journeyDate) - 1);
    schedule->status = status;

    schedule->prev = nullptr;
    schedule->next = scheduleHead;
    if (scheduleHead != nullptr) {
        scheduleHead->prev = schedule;
    }
    scheduleHead = schedule;
    return schedule;
}

class ThreadedTrainSearchTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        clearTrainList();
        clearScheduleList();
    }

    void TearDown() override
    {
        clearTrainList();
        clearScheduleList();
    }
};

TEST_F(ThreadedTrainSearchTest, RejectsNullSource)
{
    int results[4] = {};
    EXPECT_EQ(threadedTrainSearch(nullptr, "Bengaluru", nullptr,
                                  results, 4), -1);
}

TEST_F(ThreadedTrainSearchTest, RejectsNullDestination)
{
    int results[4] = {};
    EXPECT_EQ(threadedTrainSearch("Chennai", nullptr, nullptr,
                                  results, 4), -1);
}

TEST_F(ThreadedTrainSearchTest, RejectsNullOutputBuffer)
{
    EXPECT_EQ(threadedTrainSearch("Chennai", "Bengaluru", nullptr,
                                  nullptr, 4), -1);
}

TEST_F(ThreadedTrainSearchTest, RejectsZeroResultCapacity)
{
    int results[4] = {};
    EXPECT_EQ(threadedTrainSearch("Chennai", "Bengaluru", nullptr,
                                  results, 0), -1);
}

TEST_F(ThreadedTrainSearchTest, EmptyTrainListReturnsZero)
{
    int results[4] = {};
    EXPECT_EQ(threadedTrainSearch("Chennai", "Bengaluru", nullptr,
                                  results, 4), 0);
}

TEST_F(ThreadedTrainSearchTest, FindsMatchingRouteWithoutDate)
{
    ASSERT_NE(makeTrain(12345, "Chennai", "Bengaluru"), nullptr);
    int results[4] = {};

    int count = threadedTrainSearch("Chennai", "Bengaluru", nullptr,
                                    results, 4);

    ASSERT_EQ(count, 1);
    EXPECT_EQ(results[0], 12345);
}

TEST_F(ThreadedTrainSearchTest, DoesNotMatchReverseRoute)
{
    ASSERT_NE(makeTrain(12345, "Bengaluru", "Chennai"), nullptr);
    int results[4] = {};

    EXPECT_EQ(threadedTrainSearch("Chennai", "Bengaluru", nullptr,
                                  results, 4), 0);
}

TEST_F(ThreadedTrainSearchTest, DoesNotMatchUnknownStation)
{
    ASSERT_NE(makeTrain(12345, "Chennai", "Bengaluru"), nullptr);
    int results[4] = {};

    EXPECT_EQ(threadedTrainSearch("Madurai", "Bengaluru", nullptr,
                                  results, 4), 0);
}

TEST_F(ThreadedTrainSearchTest, RejectsCapacitySmallerThanTrainCount)
{
    ASSERT_NE(makeTrain(12345, "Chennai", "Bengaluru"), nullptr);
    ASSERT_NE(makeTrain(12346, "Chennai", "Mysuru"), nullptr);
    int results[1] = {};

    EXPECT_EQ(threadedTrainSearch("Chennai", "Bengaluru", nullptr,
                                  results, 1), -1);
}

TEST_F(ThreadedTrainSearchTest, ReturnsMatchingTrainsInListOrder)
{
    ASSERT_NE(makeTrain(12345, "Chennai", "Bengaluru"), nullptr);
    ASSERT_NE(makeTrain(12346, "Chennai", "Mysuru"), nullptr);
    ASSERT_NE(makeTrain(12347, "Chennai", "Bengaluru"), nullptr);
    int results[3] = {};

    int count = threadedTrainSearch("Chennai", "Bengaluru", nullptr,
                                    results, 3);

    ASSERT_EQ(count, 2);
    EXPECT_EQ(results[0], 12347);
    EXPECT_EQ(results[1], 12345);
}

TEST_F(ThreadedTrainSearchTest, DatedSearchRequiresMatchingSchedule)
{
    ASSERT_NE(makeTrain(12345, "Chennai", "Bengaluru"), nullptr);
    ASSERT_NE(makeSchedule(12345, "25:12:2026"), nullptr);
    int results[4] = {};

    EXPECT_EQ(threadedTrainSearch("Chennai", "Bengaluru", "25:12:2026",
                                  results, 4), 1);
}

TEST_F(ThreadedTrainSearchTest, DatedSearchRejectsMissingSchedule)
{
    ASSERT_NE(makeTrain(12345, "Chennai", "Bengaluru"), nullptr);
    int results[4] = {};

    EXPECT_EQ(threadedTrainSearch("Chennai", "Bengaluru", "25:12:2026",
                                  results, 4), 0);
}

TEST_F(ThreadedTrainSearchTest, DatedSearchRejectsDifferentScheduleDate)
{
    ASSERT_NE(makeTrain(12345, "Chennai", "Bengaluru"), nullptr);
    ASSERT_NE(makeSchedule(12345, "26:12:2026"), nullptr);
    int results[4] = {};

    EXPECT_EQ(threadedTrainSearch("Chennai", "Bengaluru", "25:12:2026",
                                  results, 4), 0);
}

TEST_F(ThreadedTrainSearchTest, DatedSearchRejectsCancelledSchedule)
{
    ASSERT_NE(makeTrain(12345, "Chennai", "Bengaluru"), nullptr);
    ASSERT_NE(makeSchedule(12345, "25:12:2026", TRAIN_CANCELLED), nullptr);
    int results[4] = {};

    EXPECT_EQ(threadedTrainSearch("Chennai", "Bengaluru", "25:12:2026",
                                  results, 4), 0);
}

TEST_F(ThreadedTrainSearchTest, IgnoresScheduleForAnotherTrain)
{
    ASSERT_NE(makeTrain(12345, "Chennai", "Bengaluru"), nullptr);
    ASSERT_NE(makeSchedule(99999, "25:12:2026"), nullptr);
    int results[4] = {};

    EXPECT_EQ(threadedTrainSearch("Chennai", "Bengaluru", "25:12:2026",
                                  results, 4), 0);
}

TEST_F(ThreadedTrainSearchTest, EqualSourceAndDestinationDoesNotMatch)
{
    ASSERT_NE(makeTrain(12345, "Chennai", "Bengaluru"), nullptr);
    int results[4] = {};

    EXPECT_EQ(threadedTrainSearch("Chennai", "Chennai", nullptr,
                                  results, 4), 0);
}

TEST_F(ThreadedTrainSearchTest, SupportsMoreThanOneTrain)
{
    ASSERT_NE(makeTrain(12345, "Chennai", "Bengaluru"), nullptr);
    ASSERT_NE(makeTrain(12346, "Chennai", "Bengaluru"), nullptr);
    int results[2] = {};

    EXPECT_EQ(threadedTrainSearch("Chennai", "Bengaluru", nullptr,
                                  results, 2), 2);
}

}  // namespace
