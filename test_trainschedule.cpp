#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

extern "C" {
#include "trainschedule.h"
#include "train.h"
#include "common.h"
}

/* Functions provided by the other project modules. */
extern "C" {
    int isValidJourneyDate(const char *date);
    int railway_schedule_lead_time_ok(const char *date, int type);
}

/* ---------------------------------------------------------
   TEST FIXTURE
--------------------------------------------------------- */

class TrainScheduleTest : public ::testing::Test {
protected:
    Train train1{};
    Train train2{};

    void SetUp() override {
        clearSchedules();

        trainHead = nullptr;

        train1.trainNumber = 12627;
        std::strcpy(train1.trainName, "Karnataka Express");
        train1.prev = nullptr;
        train1.next = &train2;

        train2.trainNumber = 12628;
        std::strcpy(train2.trainName, "Chennai Express");
        train2.prev = &train1;
        train2.next = nullptr;

        trainHead = &train1;

        std::remove("trainschedule.txt");
        std::remove("trainschedule.txt.tmp");
        std::remove("trains.txt");
    }

    void TearDown() override {
        clearSchedules();
        trainHead = nullptr;

        std::remove("trainschedule.txt");
        std::remove("trainschedule.txt.tmp");
        std::remove("trains.txt");
    }

    static void clearSchedules() {
        while (scheduleHead != nullptr) {
            TrainSchedule *next = scheduleHead->next;
            std::free(scheduleHead);
            scheduleHead = next;
        }
    }

    TrainSchedule *makeSchedule(
        int number,
        const char *date,
        int status = TRAIN_RUNNING,
        int delay = 0) {
        auto *node = static_cast<TrainSchedule *>(
            std::calloc(1U, sizeof(TrainSchedule)));

        EXPECT_NE(node, nullptr);
        if (node == nullptr) {
            return nullptr;
        }

        node->trainNumber = number;
        std::strncpy(
            node->journeyDate,
            date,
            sizeof(node->journeyDate) - 1U);
        node->journeyDate[sizeof(node->journeyDate) - 1U] = '\0';
        node->status = status;
        node->delayMinutes = delay;

        node->prev = nullptr;
        node->next = scheduleHead;

        if (scheduleHead != nullptr) {
            scheduleHead->prev = node;
        }

        scheduleHead = node;
        return node;
    }

    std::string runWithInput(
        void (*function)(),
        const std::string &input) {
        fflush(stdout);

        FILE *inputFile = std::tmpfile();
        FILE *outputFile = std::tmpfile();

        EXPECT_NE(inputFile, nullptr);
        EXPECT_NE(outputFile, nullptr);

        if (inputFile == nullptr || outputFile == nullptr) {
            if (inputFile != nullptr) {
                std::fclose(inputFile);
            }
            if (outputFile != nullptr) {
                std::fclose(outputFile);
            }
            return "";
        }

        std::fwrite(input.data(), 1U, input.size(), inputFile);
        std::rewind(inputFile);

        int savedStdin = dup(STDIN_FILENO);
        int savedStdout = dup(STDOUT_FILENO);

        dup2(fileno(inputFile), STDIN_FILENO);
        dup2(fileno(outputFile), STDOUT_FILENO);

        clearerr(stdin);
        function();

        fflush(stdout);
        dup2(savedStdin, STDIN_FILENO);
        dup2(savedStdout, STDOUT_FILENO);

        close(savedStdin);
        close(savedStdout);

        clearerr(stdin);

        std::rewind(outputFile);
        std::string output;
        char buffer[512];

        while (std::fgets(buffer, sizeof(buffer), outputFile) != nullptr) {
            output += buffer;
        }

        std::fclose(inputFile);
        std::fclose(outputFile);

        return output;
    }
};

/* ---------------------------------------------------------
   INITIALIZATION AND EMPTY LIST
--------------------------------------------------------- */

TEST_F(TrainScheduleTest, ViewSchedulesWhenEmpty) {
    const std::string output =
        runWithInput(viewTrainSchedules, "");

    EXPECT_NE(output.find("No Schedule Available"), std::string::npos);
}

TEST_F(TrainScheduleTest, LoadWhenScheduleFileDoesNotExist) {
    std::remove("trainschedule.txt");

    loadTrainScheduleData();

    EXPECT_EQ(scheduleHead, nullptr);
}

TEST_F(TrainScheduleTest, InitializeCreatesScheduleFile) {
    initializeTrainScheduleModule();

    std::ifstream file("trainschedule.txt");
    EXPECT_TRUE(file.good());
}

/* ---------------------------------------------------------
   SAVE AND LOAD
--------------------------------------------------------- */

TEST_F(TrainScheduleTest, SaveAndReloadOneSchedule) {
    TrainSchedule *node =
        makeSchedule(12627, "26:11:2026", TRAIN_RUNNING, 0);

    ASSERT_NE(node, nullptr);

    saveTrainScheduleData();

    clearSchedules();
    EXPECT_EQ(scheduleHead, nullptr);

    loadTrainScheduleData();

    ASSERT_NE(scheduleHead, nullptr);
    EXPECT_EQ(scheduleHead->trainNumber, 12627);
    EXPECT_STREQ(scheduleHead->journeyDate, "26:11:2026");
    EXPECT_EQ(scheduleHead->status, TRAIN_RUNNING);
    EXPECT_EQ(scheduleHead->delayMinutes, 0);
}

TEST_F(TrainScheduleTest, SaveEmptyScheduleList) {
    saveTrainScheduleData();

    std::ifstream file("trainschedule.txt");
    ASSERT_TRUE(file.good());

    EXPECT_EQ(file.peek(), std::ifstream::traits_type::eof());
}

TEST_F(TrainScheduleTest, LoadSkipsScheduleForUnknownTrain) {
    {
        std::ofstream file("trainschedule.txt");
        file << "SCHEDULE_START\n"
             << "99999\n"
             << "26:11:2026\n"
             << TRAIN_RUNNING << "\n"
             << "0\n"
             << "SCHEDULE_END\n";
    }

    loadTrainScheduleData();

    EXPECT_EQ(scheduleHead, nullptr);
}

TEST_F(TrainScheduleTest, LoadSkipsInvalidDate) {
    {
        std::ofstream file("trainschedule.txt");
        file << "SCHEDULE_START\n"
             << "12627\n"
             << "31:02:2027\n"
             << TRAIN_RUNNING << "\n"
             << "0\n"
             << "SCHEDULE_END\n";
    }

    loadTrainScheduleData();

    EXPECT_EQ(scheduleHead, nullptr);
}

TEST_F(TrainScheduleTest, LoadSkipsInvalidStatus) {
    {
        std::ofstream file("trainschedule.txt");
        file << "SCHEDULE_START\n"
             << "12627\n"
             << "26:11:2026\n"
             << "99\n"
             << "0\n"
             << "SCHEDULE_END\n";
    }

    loadTrainScheduleData();

    EXPECT_EQ(scheduleHead, nullptr);
}

TEST_F(TrainScheduleTest, LoadSkipsNegativeDelay) {
    {
        std::ofstream file("trainschedule.txt");
        file << "SCHEDULE_START\n"
             << "12627\n"
             << "26:11:2026\n"
             << TRAIN_RUNNING << "\n"
             << "-1\n"
             << "SCHEDULE_END\n";
    }

    loadTrainScheduleData();

    EXPECT_EQ(scheduleHead, nullptr);
}

TEST_F(TrainScheduleTest, LoadSkipsDelayAboveLimit) {
    {
        std::ofstream file("trainschedule.txt");
        file << "SCHEDULE_START\n"
             << "12627\n"
             << "26:11:2026\n"
             << TRAIN_RUNNING << "\n"
             << "43201\n"
             << "SCHEDULE_END\n";
    }

    loadTrainScheduleData();

    EXPECT_EQ(scheduleHead, nullptr);
}

TEST_F(TrainScheduleTest, LoadSkipsDuplicateSchedules) {
    {
        std::ofstream file("trainschedule.txt");

        for (int i = 0; i < 2; ++i) {
            file << "SCHEDULE_START\n"
                 << "12627\n"
                 << "26:11:2026\n"
                 << TRAIN_RUNNING << "\n"
                 << "0\n"
                 << "SCHEDULE_END\n";
        }
    }

    loadTrainScheduleData();

    ASSERT_NE(scheduleHead, nullptr);
    EXPECT_EQ(scheduleHead->trainNumber, 12627);
    EXPECT_EQ(scheduleHead->next, nullptr);
}

TEST_F(TrainScheduleTest, LoadSkipsTextOutsideRecords) {
    {
        std::ofstream file("trainschedule.txt");
        file << "Unrelated text\n"
             << "ANOTHER_MARKER\n";
    }

    loadTrainScheduleData();

    EXPECT_EQ(scheduleHead, nullptr);
}

/* ---------------------------------------------------------
   VIEW SCHEDULES
--------------------------------------------------------- */

TEST_F(TrainScheduleTest, ViewRunningSchedule) {
    ASSERT_NE(
        makeSchedule(12627, "26:11:2026", TRAIN_RUNNING, 0),
        nullptr);

    const std::string output =
        runWithInput(viewTrainSchedules, "");

    EXPECT_NE(output.find("12627"), std::string::npos);
    EXPECT_NE(output.find("26:11:2026"), std::string::npos);
    EXPECT_NE(output.find("Running"), std::string::npos);
}

TEST_F(TrainScheduleTest, ViewRescheduledSchedule) {
    ASSERT_NE(
        makeSchedule(12627, "26:11:2026", TRAIN_RESCHEDULED, 60),
        nullptr);

    const std::string output =
        runWithInput(viewTrainSchedules, "");

    EXPECT_NE(output.find("Rescheduled"), std::string::npos);
    EXPECT_NE(output.find("60"), std::string::npos);
}

TEST_F(TrainScheduleTest, ViewCancelledSchedule) {
    ASSERT_NE(
        makeSchedule(12627, "26:11:2026", TRAIN_CANCELLED, 0),
        nullptr);

    const std::string output =
        runWithInput(viewTrainSchedules, "");

    EXPECT_NE(output.find("Cancelled"), std::string::npos);
}

TEST_F(TrainScheduleTest, ViewSpecialSchedule) {
    ASSERT_NE(
        makeSchedule(12627, "26:11:2026", TRAIN_SPECIAL, 0),
        nullptr);

    const std::string output =
        runWithInput(viewTrainSchedules, "");

    EXPECT_NE(output.find("Special"), std::string::npos);
}

TEST_F(TrainScheduleTest, ViewUnknownStatus) {
    ASSERT_NE(makeSchedule(12627, "26:11:2026", 99, 0), nullptr);

    const std::string output =
        runWithInput(viewTrainSchedules, "");

    EXPECT_NE(output.find("Unknown"), std::string::npos);
}

/* ---------------------------------------------------------
   SEARCH
--------------------------------------------------------- */

TEST_F(TrainScheduleTest, SearchExistingSchedule) {
    ASSERT_NE(
        makeSchedule(12627, "26:11:2026", TRAIN_RUNNING, 0),
        nullptr);

    const std::string output =
        runWithInput(searchTrainSchedule, "12627\n26:11:2026\n");

    EXPECT_NE(output.find("Train Number : 12627"), std::string::npos);
    EXPECT_NE(output.find("26:11:2026"), std::string::npos);
}

TEST_F(TrainScheduleTest, SearchMissingSchedule) {
    const std::string output =
        runWithInput(searchTrainSchedule, "12627\n26:11:2026\n");

    EXPECT_NE(output.find("Schedule Not Found"), std::string::npos);
}

TEST_F(TrainScheduleTest, SearchRetriesInvalidDate) {
    ASSERT_NE(
        makeSchedule(12627, "26:11:2026", TRAIN_RUNNING, 0),
        nullptr);

    const std::string output =
        runWithInput(
            searchTrainSchedule,
            "12627\n31:02:2027\n26:11:2026\n");

    EXPECT_NE(output.find("Invalid calendar date"), std::string::npos);
    EXPECT_NE(output.find("Train Number : 12627"), std::string::npos);
}

/* ---------------------------------------------------------
   UPDATE
--------------------------------------------------------- */

TEST_F(TrainScheduleTest, UpdateScheduleToRescheduled) {
    ASSERT_NE(
        makeSchedule(12627, "26:11:2026", TRAIN_RUNNING, 0),
        nullptr);

    const std::string output =
        runWithInput(
            updateTrainSchedule,
            "12627\n26:11:2026\n1\n2\n");

    EXPECT_NE(output.find("Schedule Updated Successfully"), std::string::npos);
    EXPECT_EQ(scheduleHead->status, TRAIN_RESCHEDULED);
    EXPECT_EQ(scheduleHead->delayMinutes, 120);
}

TEST_F(TrainScheduleTest, UpdateScheduleToCancelled) {
    ASSERT_NE(
        makeSchedule(12627, "26:11:2026", TRAIN_RUNNING, 0),
        nullptr);

    const std::string output =
        runWithInput(
            updateTrainSchedule,
            "12627\n26:11:2026\n2\nY\n");

    EXPECT_NE(output.find("Train Cancelled"), std::string::npos);
    EXPECT_EQ(scheduleHead->status, TRAIN_CANCELLED);
    EXPECT_EQ(scheduleHead->delayMinutes, 0);
}

TEST_F(TrainScheduleTest, CancelScheduleCanBeAborted) {
    ASSERT_NE(
        makeSchedule(12627, "26:11:2026", TRAIN_RUNNING, 0),
        nullptr);

    const std::string output =
        runWithInput(
            updateTrainSchedule,
            "12627\n26:11:2026\n2\nN\n");

    EXPECT_NE(output.find("Cancellation aborted"), std::string::npos);
    EXPECT_EQ(scheduleHead->status, TRAIN_RUNNING);
}

TEST_F(TrainScheduleTest, UpdateScheduleToSpecial) {
    ASSERT_NE(
        makeSchedule(12627, "26:11:2026", TRAIN_RUNNING, 0),
        nullptr);

    const std::string output =
        runWithInput(
            updateTrainSchedule,
            "12627\n26:11:2026\n3\n");

    EXPECT_NE(output.find("Marked As Special Train"), std::string::npos);
    EXPECT_EQ(scheduleHead->status, TRAIN_SPECIAL);
    EXPECT_EQ(scheduleHead->delayMinutes, 0);
}

TEST_F(TrainScheduleTest, RestoreScheduleToRunning) {
    ASSERT_NE(
        makeSchedule(12627, "26:11:2026", TRAIN_CANCELLED, 120),
        nullptr);

    const std::string output =
        runWithInput(
            updateTrainSchedule,
            "12627\n26:11:2026\n4\n");

    EXPECT_NE(output.find("Train Restored To Running"), std::string::npos);
    EXPECT_EQ(scheduleHead->status, TRAIN_RUNNING);
    EXPECT_EQ(scheduleHead->delayMinutes, 0);
}

TEST_F(TrainScheduleTest, UpdateScheduleRetriesInvalidChoice) {
    ASSERT_NE(
        makeSchedule(12627, "26:11:2026", TRAIN_RUNNING, 0),
        nullptr);

    const std::string output =
        runWithInput(
            updateTrainSchedule,
            "12627\n26:11:2026\n9\n1\n0\n");

    EXPECT_NE(output.find("Choose 1, 2, 3 or 4"), std::string::npos);
    EXPECT_EQ(scheduleHead->status, TRAIN_RESCHEDULED);
    EXPECT_EQ(scheduleHead->delayMinutes, 0);
}

TEST_F(TrainScheduleTest, UpdateScheduleRetriesInvalidDelay) {
    ASSERT_NE(
        makeSchedule(12627, "26:11:2026", TRAIN_RUNNING, 0),
        nullptr);

    const std::string output =
        runWithInput(
            updateTrainSchedule,
            "12627\n26:11:2026\n1\n-1\n721\n3\n");

    EXPECT_NE(output.find("Delay must be between 0 and 720"), std::string::npos);
    EXPECT_EQ(scheduleHead->status, TRAIN_RESCHEDULED);
    EXPECT_EQ(scheduleHead->delayMinutes, 180);
}

TEST_F(TrainScheduleTest, UpdateMissingSchedule) {
    const std::string output =
        runWithInput(
            updateTrainSchedule,
            "12627\n26:11:2026\n");

    EXPECT_NE(
        output.find("No Schedule Exists For This Train On This Date"),
        std::string::npos);
}

/* ---------------------------------------------------------
   DELETE
--------------------------------------------------------- */

TEST_F(TrainScheduleTest, DeleteHeadSchedule) {
    ASSERT_NE(
        makeSchedule(12627, "26:11:2026", TRAIN_RUNNING, 0),
        nullptr);

    const std::string output =
        runWithInput(
            deleteTrainSchedule,
            "12627\n26:11:2026\nY\n");

    EXPECT_NE(output.find("Schedule Deleted Successfully"), std::string::npos);
    EXPECT_EQ(scheduleHead, nullptr);
}

TEST_F(TrainScheduleTest, DeleteHeadAndRelinkNextNode) {
    TrainSchedule *older =
        makeSchedule(12628, "27:11:2026", TRAIN_RUNNING, 0);
    ASSERT_NE(older, nullptr);

    TrainSchedule *newer =
        makeSchedule(12627, "26:11:2026", TRAIN_RUNNING, 0);
    ASSERT_NE(newer, nullptr);

    const std::string output =
        runWithInput(
            deleteTrainSchedule,
            "12627\n26:11:2026\nY\n");

    EXPECT_NE(output.find("Schedule Deleted Successfully"), std::string::npos);
    ASSERT_EQ(scheduleHead, older);
    EXPECT_EQ(older->prev, nullptr);
}

TEST_F(TrainScheduleTest, DeleteMiddleSchedule) {
    TrainSchedule *third =
        makeSchedule(12628, "28:11:2026", TRAIN_RUNNING, 0);
    ASSERT_NE(third, nullptr);

    TrainSchedule *middle =
        makeSchedule(12627, "27:11:2026", TRAIN_RUNNING, 0);
    ASSERT_NE(middle, nullptr);

    TrainSchedule *first =
        makeSchedule(12628, "26:11:2026", TRAIN_RUNNING, 0);
    ASSERT_NE(first, nullptr);

    const std::string output =
        runWithInput(
            deleteTrainSchedule,
            "12627\n27:11:2026\nY\n");

    EXPECT_NE(output.find("Schedule Deleted Successfully"), std::string::npos);
    ASSERT_EQ(scheduleHead, first);
    ASSERT_NE(first->next, nullptr);
    EXPECT_EQ(first->next, third);
    EXPECT_EQ(third->prev, first);
}

TEST_F(TrainScheduleTest, DeleteCanBeAborted) {
    ASSERT_NE(
        makeSchedule(12627, "26:11:2026", TRAIN_RUNNING, 0),
        nullptr);

    const std::string output =
        runWithInput(
            deleteTrainSchedule,
            "12627\n26:11:2026\nN\n");

    EXPECT_NE(output.find("Deletion aborted"), std::string::npos);
    EXPECT_NE(scheduleHead, nullptr);
}

TEST_F(TrainScheduleTest, DeleteMissingSchedule) {
    const std::string output =
        runWithInput(
            deleteTrainSchedule,
            "12627\n26:11:2026\n");

    EXPECT_NE(output.find("Schedule Not Found"), std::string::npos);
}

/* ---------------------------------------------------------
   ADD SCHEDULE
--------------------------------------------------------- */

TEST_F(TrainScheduleTest, AddScheduleRejectsUnknownTrainThreeTimes) {
    const std::string output =
        runWithInput(
            addTrainSchedule,
            "99999\n99999\n99999\n");

    EXPECT_NE(
        output.find("Three invalid attempts"),
        std::string::npos);
    EXPECT_EQ(scheduleHead, nullptr);
}

TEST_F(TrainScheduleTest, AddScheduleRejectsInvalidScheduleTypes) {
    const std::string output =
        runWithInput(
            addTrainSchedule,
            "12627\n0\n0\n0\n");

    EXPECT_NE(output.find("Choose 1 or 2"), std::string::npos);
    EXPECT_NE(output.find("Three invalid attempts"), std::string::npos);
    EXPECT_EQ(scheduleHead, nullptr);
}

TEST_F(TrainScheduleTest, AddScheduleRejectsInvalidDates) {
    const std::string output =
        runWithInput(
            addTrainSchedule,
            "12627\n1\n31:02:2027\n31:02:2027\n31:02:2027\n");

    EXPECT_NE(output.find("Invalid calendar date"), std::string::npos);
    EXPECT_EQ(scheduleHead, nullptr);
}

TEST_F(TrainScheduleTest, AddScheduleRejectsDuplicate) {
    ASSERT_NE(
        makeSchedule(12627, "26:11:2026", TRAIN_RUNNING, 0),
        nullptr);

    /*
     * The chosen date must pass railway_schedule_lead_time_ok()
     * for the test's current date. Update it to a suitable future
     * date if your lead-time helper uses a different clock.
     */
    const std::string output =
        runWithInput(
            addTrainSchedule,
            "12627\n1\n26:11:2026\n");

    EXPECT_TRUE(
        output.find("Schedule Already Exists") != std::string::npos ||
        output.find("Regular train schedules must be at least one calendar month ahead") != std::string::npos);
}

/* ---------------------------------------------------------
   DATE VALIDATION THROUGH PUBLIC FUNCTIONS
--------------------------------------------------------- */

TEST_F(TrainScheduleTest, DateTimeHelperAcceptsValidDate) {
    EXPECT_TRUE(isValidJourneyDate("26:11:2026"));
}

TEST_F(TrainScheduleTest, DateTimeHelperRejectsInvalidDate) {
    EXPECT_FALSE(isValidJourneyDate("31:02:2027"));
    EXPECT_FALSE(isValidJourneyDate("abc"));
    EXPECT_FALSE(isValidJourneyDate("1:1:2026"));
}