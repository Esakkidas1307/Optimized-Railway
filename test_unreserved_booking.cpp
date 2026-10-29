#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <ctime>

extern "C" {
#include "common.h"
#include "constants.h"
#include "passenger.h"
#include "unreserved_booking.h"
#include "reserved_booking.h"
#include "train.h"
#include "trainschedule.h"
#include "utils.h"
#include "availability.h"
#include "date_time.h"
}
extern "C" {
    Passenger *passengerHead = nullptr;
}
namespace {

class UnreservedBookingTest : public ::testing::Test {
protected:
    const char *inputFile = "test_unreserved_stdin.txt";

    void SetUp() override {
        cleanupData();
    }

    void TearDown() override {
        // Restore stdin so other Google Tests are not affected.
        if (stdin) {
            clearerr(stdin);
        }
        cleanupData();
    }

    void cleanupData() {
        while (bookingHead != nullptr) {
            Booking *next = bookingHead->next;
            free(bookingHead);
            bookingHead = next;
        }

        while (trainHead != nullptr) {
            Train *next = trainHead->next;
            free(trainHead);
            trainHead = next;
        }

        while (scheduleHead != nullptr) {
            TrainSchedule *next = scheduleHead->next;
            free(scheduleHead);
            scheduleHead = next;
        }

        bookingHead = nullptr;
        trainHead = nullptr;
        scheduleHead = nullptr;
        remove(inputFile);
    }

    void feedStdin(const std::string &input) {
        FILE *file = fopen(inputFile, "w");
        ASSERT_NE(file, nullptr);

        fputs(input.c_str(), file);
        fclose(file);

        ASSERT_NE(freopen(inputFile, "r", stdin), nullptr);
    }

    Train *createTestTrain(int number, const char *name) {
        Train *train =
            static_cast<Train *>(calloc(1, sizeof(Train)));
        if (!train) return nullptr;

        train->trainNumber = number;
        snprintf(train->trainName, sizeof(train->trainName), "%s", name);
        train->totalStations = 3;

        snprintf(train->stations[0].stationName,
                 sizeof(train->stations[0].stationName), "DELHI");
        snprintf(train->stations[0].departureTime,
                 sizeof(train->stations[0].departureTime), "23:59");
        train->stations[0].distanceFromSource = 0;

        snprintf(train->stations[1].stationName,
                 sizeof(train->stations[1].stationName), "KANPUR");
        snprintf(train->stations[1].departureTime,
                 sizeof(train->stations[1].departureTime), "23:59");
        train->stations[1].distanceFromSource = 440;

        snprintf(train->stations[2].stationName,
                 sizeof(train->stations[2].stationName), "PRAYAGRAJ");
        snprintf(train->stations[2].departureTime,
                 sizeof(train->stations[2].departureTime), "23:59");
        train->stations[2].distanceFromSource = 640;

        train->next = trainHead;
        if (trainHead) trainHead->prev = train;
        trainHead = train;

        return train;
    }

    TrainSchedule *createTestSchedule(
        int number, const char *date, int status) {
        TrainSchedule *schedule =
            static_cast<TrainSchedule *>(calloc(1, sizeof(TrainSchedule)));
        if (!schedule) return nullptr;

        schedule->trainNumber = number;
        snprintf(schedule->journeyDate,
                 sizeof(schedule->journeyDate), "%s", date);
        schedule->status = status;

        schedule->next = scheduleHead;
        if (scheduleHead) scheduleHead->prev = schedule;
        scheduleHead = schedule;

        return schedule;
    }

    std::string today() {
        char buffer[11] = {};
        time_t now = time(nullptr);
        struct tm localNow {};
        localtime_r(&now, &localNow);
        strftime(buffer, sizeof(buffer), "%d:%m:%Y", &localNow);
        return buffer;
    }

    void addTodaySchedule(int number, int status = TRAIN_RUNNING) {
        ASSERT_NE(createTestSchedule(number, today().c_str(), status), nullptr);
    }
};

TEST_F(UnreservedBookingTest, CancelsAtSourcePrompt) {
    feedStdin("0\n");

    testing::internal::CaptureStdout();
    bookUnreservedTicket(101);
    testing::internal::GetCapturedStdout();

    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(UnreservedBookingTest, CancelsAtDestinationPrompt) {
    feedStdin("DELHI\n0\n");

    testing::internal::CaptureStdout();
    bookUnreservedTicket(101);
    testing::internal::GetCapturedStdout();

    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(UnreservedBookingTest, RejectsSameSourceAndDestination) {
    feedStdin("DELHI\nDELHI\n");

    testing::internal::CaptureStdout();
    bookUnreservedTicket(101);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Source and destination must differ."),
              std::string::npos);
    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(UnreservedBookingTest, ReturnsWhenNoTrainsExist) {
    feedStdin("DELHI\nKANPUR\n");

    testing::internal::CaptureStdout();
    bookUnreservedTicket(101);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("No future train found for this route today."),
              std::string::npos);
    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(UnreservedBookingTest, RejectsReverseRoute) {
    ASSERT_NE(createTestTrain(12001, "Test Express"), nullptr);
    addTodaySchedule(12001);

    feedStdin("PRAYAGRAJ\nDELHI\n");

    testing::internal::CaptureStdout();
    bookUnreservedTicket(101);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("No future train found for this route today."),
              std::string::npos);
    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(UnreservedBookingTest, RejectsUnknownStation) {
    ASSERT_NE(createTestTrain(12001, "Test Express"), nullptr);
    addTodaySchedule(12001);

    feedStdin("UNKNOWN\nKANPUR\n");

    testing::internal::CaptureStdout();
    bookUnreservedTicket(101);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("No future train found for this route today."),
              std::string::npos);
    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(UnreservedBookingTest, RejectsCancelledSchedule) {
    ASSERT_NE(createTestTrain(12001, "Test Express"), nullptr);
    addTodaySchedule(12001, TRAIN_CANCELLED);

    feedStdin("DELHI\nKANPUR\n");

    testing::internal::CaptureStdout();
    bookUnreservedTicket(101);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("No future train found for this route today."),
              std::string::npos);
    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(UnreservedBookingTest, CancelsAtTrainTypePrompt) {
    ASSERT_NE(createTestTrain(12001, "Test Express"), nullptr);
    addTodaySchedule(12001);

    feedStdin("DELHI\nKANPUR\n0\n");

    testing::internal::CaptureStdout();
    bookUnreservedTicket(101);
    testing::internal::GetCapturedStdout();

    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(UnreservedBookingTest, RejectsInvalidTrainTypeAfterThreeAttempts) {
    ASSERT_NE(createTestTrain(12001, "Test Express"), nullptr);
    addTodaySchedule(12001);

    feedStdin("DELHI\nKANPUR\n5\n5\n5\n");

    testing::internal::CaptureStdout();
    bookUnreservedTicket(101);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Choose 1 for EXP or 2 for SUPERFAST"),
              std::string::npos);
    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(UnreservedBookingTest, CancelsAtTrainNumberPrompt) {
    ASSERT_NE(createTestTrain(12001, "Test Express"), nullptr);
    addTodaySchedule(12001);

    feedStdin("DELHI\nKANPUR\n1\n0\n");

    testing::internal::CaptureStdout();
    bookUnreservedTicket(101);
    testing::internal::GetCapturedStdout();

    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(UnreservedBookingTest, RejectsInvalidTrainNumberAfterThreeAttempts) {
    ASSERT_NE(createTestTrain(12001, "Test Express"), nullptr);
    addTodaySchedule(12001);

    feedStdin("DELHI\nKANPUR\n1\n9999\n9998\n9997\n");

    testing::internal::CaptureStdout();
    bookUnreservedTicket(101);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Choose a train number from the displayed route list"),
              std::string::npos);
    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(UnreservedBookingTest, RejectsInvalidPassengerCounts) {
    ASSERT_NE(createTestTrain(12001, "Test Express"), nullptr);
    addTodaySchedule(12001);

    feedStdin(
        "DELHI\nKANPUR\n"
        "1\n12001\n"
        "-1\n0\n"
        "-1\n0\n"
        "-1\n0\n"
    );

    testing::internal::CaptureStdout();
    bookUnreservedTicket(101);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Invalid passenger counts"),
              std::string::npos);
    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(UnreservedBookingTest, RejectsZeroPassengers) {
    ASSERT_NE(createTestTrain(12001, "Test Express"), nullptr);
    addTodaySchedule(12001);

    feedStdin(
        "DELHI\nKANPUR\n"
        "1\n12001\n"
        "0\n0\n"
        "0\n0\n"
        "0\n0\n"
    );

    testing::internal::CaptureStdout();
    bookUnreservedTicket(101);
    testing::internal::GetCapturedStdout();

    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(UnreservedBookingTest, RejectsPaymentAfterThreeIncorrectAmounts) {
    ASSERT_NE(createTestTrain(12001, "Test Express"), nullptr);
    addTodaySchedule(12001);

    // One adult on the 440 km EXP route costs Rs. 220.
    feedStdin(
        "DELHI\nKANPUR\n"
        "1\n12001\n"
        "1\n0\n"
        "100\n150\n200\n"
    );

    testing::internal::CaptureStdout();
    bookUnreservedTicket(101);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Payment failed. No unreserved ticket was created."),
              std::string::npos);
    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(UnreservedBookingTest, RejectsNegativePaymentInput) {
    ASSERT_NE(createTestTrain(12001, "Test Express"), nullptr);
    addTodaySchedule(12001);

    feedStdin(
        "DELHI\nKANPUR\n"
        "1\n12001\n"
        "1\n0\n"
        "-1\n"
    );

    testing::internal::CaptureStdout();
    bookUnreservedTicket(101);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Invalid payment input exhausted"),
              std::string::npos);
    EXPECT_EQ(bookingHead, nullptr);
}

} // namespace
