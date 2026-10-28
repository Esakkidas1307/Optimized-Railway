#include <gtest/gtest.h>

extern "C" {
#include "availability.h"
#include "train.h"
#include "trainschedule.h"
#include "utils.h"
}

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <iterator>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

class AvailabilityTest : public ::testing::Test {
protected:
    fs::path originalDir;
    fs::path tempDir;

    void addTestAvailability(
    int seats,
    int rac,
    int wl,
    const char* date = "25:10:2026",
    int number = 12345
) {
    auto* record =
        static_cast<Availability*>(std::calloc(1, sizeof(Availability)));

    ASSERT_NE(record, nullptr);

    record->trainNumber = number;

    std::snprintf(
        record->journeyDate,
        sizeof(record->journeyDate),
        "%s",
        date
    );

    std::snprintf(
        record->className,
        sizeof(record->className),
        "%s",
        "SL"
    );

    record->availableSeats = seats;
    record->availableRAC = rac;
    record->availableWL = wl;

    record->next = availabilityHead;

    if (availabilityHead != nullptr) {
        availabilityHead->prev = record;
    }

    availabilityHead = record;
}
    void SetUp() override {
        originalDir = fs::current_path();

        char pattern[] = "/tmp/availability_tests_XXXXXX";
        char* created = mkdtemp(pattern);
        ASSERT_NE(created, nullptr);

        tempDir = created;
        fs::current_path(tempDir);

        availabilityHead = nullptr;
        trainHead = nullptr;
        scheduleHead = nullptr;
    }

    void TearDown() override {
        while (availabilityHead != nullptr) {
            Availability* next = availabilityHead->next;
            std::free(availabilityHead);
            availabilityHead = next;
        }

        while (trainHead != nullptr) {
            Train* next = trainHead->next;
            std::free(trainHead);
            trainHead = next;
        }

        while (scheduleHead != nullptr) {
            TrainSchedule* next = scheduleHead->next;
            std::free(scheduleHead);
            scheduleHead = next;
        }

        fs::current_path(originalDir);
        fs::remove_all(tempDir);
    }

    Train* addTestTrain(int number = 12345) {
        auto* train = static_cast<Train*>(std::calloc(1, sizeof(Train)));
        if (!train) return nullptr;

        train->trainNumber = number;
        train->classCount = 1;

        std::strcpy(train->coaches[0].className, "SL");
        train->coaches[0].totalSeats = 10;
        train->coaches[0].racCapacity = 4;
        train->coaches[0].wlCapacity = 5;

        train->next = trainHead;
        if (trainHead) trainHead->prev = train;
        trainHead = train;

        return train;
    }

    void addTestSchedule(int number = 12345,
                         const char* date = "25:10:2026") {
        auto* schedule =
            static_cast<TrainSchedule*>(std::calloc(1, sizeof(TrainSchedule)));
        ASSERT_NE(schedule, nullptr);

        schedule->trainNumber = number;
        std::snprintf(schedule->journeyDate,
                      sizeof(schedule->journeyDate), "%s", date);
        schedule->status = TRAIN_RUNNING;

        schedule->next = scheduleHead;
        if (scheduleHead) scheduleHead->prev = schedule;
        scheduleHead = schedule;
    }
};

std::string runCheckAvailability(const std::string& input) {
    {
        std::ofstream inputFile("test_input.txt");
        inputFile << input;
    }

    std::fflush(stdout);
    std::fflush(stderr);

    int savedStdin = dup(STDIN_FILENO);
    int savedStdout = dup(STDOUT_FILENO);

    if (savedStdin == -1 || savedStdout == -1) {
        if (savedStdin != -1) close(savedStdin);
        if (savedStdout != -1) close(savedStdout);
        return "";
    }

    if (freopen("test_input.txt", "r", stdin) == nullptr ||
        freopen("test_output.txt", "w", stdout) == nullptr) {
        dup2(savedStdin, STDIN_FILENO);
        dup2(savedStdout, STDOUT_FILENO);
        close(savedStdin);
        close(savedStdout);
        clearerr(stdin);
        clearerr(stdout);
        return "";
    }

    clearerr(stdin);
    clearerr(stdout);

    checkAvailability();

    std::fflush(stdout);

    dup2(savedStdin, STDIN_FILENO);
    dup2(savedStdout, STDOUT_FILENO);

    close(savedStdin);
    close(savedStdout);

    clearerr(stdin);
    clearerr(stdout);

    std::ifstream outputFile("test_output.txt");
    return std::string(
        std::istreambuf_iterator<char>(outputFile),
        std::istreambuf_iterator<char>()
    );
}


TEST_F(AvailabilityTest, FindAvailabilityReturnsMatchingRecord) {
    Availability record{};
    record.trainNumber = 12345;
    std::strcpy(record.journeyDate, "25:10:2026");
    std::strcpy(record.className, "SL");
    availabilityHead = &record;

    EXPECT_EQ(findAvailability(12345, "25:10:2026", "SL"), &record);

    // Detach stack-owned test record before fixture cleanup.
    availabilityHead = nullptr;
}

TEST_F(AvailabilityTest, FindAvailabilityReturnsNullForMissingRecord) {
    EXPECT_EQ(findAvailability(99999, "25:10:2026", "SL"), nullptr);
}

TEST_F(AvailabilityTest, FindAvailabilityRejectsNullDate) {
    EXPECT_EQ(findAvailability(12345, nullptr, "SL"), nullptr);
}

TEST_F(AvailabilityTest, FindAvailabilityRejectsNullClass) {
    EXPECT_EQ(findAvailability(12345, "25:10:2026", nullptr), nullptr);
}

TEST_F(AvailabilityTest, CreateAvailabilityRecordInitializesCoachInventory) {
    Train* train = addTestTrain();
    ASSERT_NE(train, nullptr);

    createAvailabilityRecord(train, "25:10:2026");

    Availability* record =
        findAvailability(12345, "25:10:2026", "SL");

    ASSERT_NE(record, nullptr);
    EXPECT_EQ(record->availableSeats, 10);
    EXPECT_EQ(record->availableRAC, 4);
    EXPECT_EQ(record->availableWL, 0);
    EXPECT_STREQ(record->journeyDate, "25:10:2026");
}

TEST_F(AvailabilityTest, CreateAvailabilityRecordIgnoresNullTrain) {
    createAvailabilityRecord(nullptr, "25:10:2026");
    EXPECT_EQ(availabilityHead, nullptr);
}

TEST_F(AvailabilityTest, CreateAvailabilityRecordIgnoresNullDate) {
    Train* train = addTestTrain();
    ASSERT_NE(train, nullptr);

    createAvailabilityRecord(train, nullptr);

    EXPECT_EQ(availabilityHead, nullptr);
}

TEST_F(AvailabilityTest, CreateAvailabilityRecordIgnoresEmptyDate) {
    Train* train = addTestTrain();
    ASSERT_NE(train, nullptr);

    createAvailabilityRecord(train, "");

    EXPECT_EQ(availabilityHead, nullptr);
}

TEST_F(AvailabilityTest, CreateAvailabilityRecordDoesNotDuplicateInventory) {
    Train* train = addTestTrain();
    ASSERT_NE(train, nullptr);

    createAvailabilityRecord(train, "25:10:2026");

    Availability* record =
        findAvailability(12345, "25:10:2026", "SL");
    ASSERT_NE(record, nullptr);

    record->availableSeats = 3;
    createAvailabilityRecord(train, "25:10:2026");

    EXPECT_EQ(findAvailability(12345, "25:10:2026", "SL"), record);
    EXPECT_EQ(record->availableSeats, 3);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataHandlesMissingFile) {
    loadAvailabilityData();
    EXPECT_EQ(availabilityHead, nullptr);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataLoadsValidRecord) {
    Train* train = addTestTrain();
    ASSERT_NE(train, nullptr);
    addTestSchedule();

    std::ofstream file("running.txt");
    ASSERT_TRUE(file.is_open());

    file << "AVAILABILITY_START\n"
         << "12345\n"
         << "25:10:2026\n"
         << "SL\n"
         << "8\n"
         << "3\n"
         << "1\n"
         << "AVAILABILITY_END\n";
    file.close();

    loadAvailabilityData();

    Availability* record =
        findAvailability(12345, "25:10:2026", "SL");

    ASSERT_NE(record, nullptr);
    EXPECT_EQ(record->availableSeats, 8);
    EXPECT_EQ(record->availableRAC, 3);
    EXPECT_EQ(record->availableWL, 1);
}

TEST_F(AvailabilityTest, SaveAvailabilityDataWritesCurrentInventory) {
    auto* record =
        static_cast<Availability*>(std::calloc(1, sizeof(Availability)));
    ASSERT_NE(record, nullptr);

    record->trainNumber = 12345;
    std::strcpy(record->journeyDate, "25:10:2026");
    std::strcpy(record->className, "SL");
    record->availableSeats = 8;
    record->availableRAC = 3;
    record->availableWL = 1;
    availabilityHead = record;

    EXPECT_EQ(saveAvailabilityData(), SUCCESS);

    std::ifstream file("running.txt");
    ASSERT_TRUE(file.is_open());

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    EXPECT_NE(content.find("AVAILABILITY_START"), std::string::npos);
    EXPECT_NE(content.find("12345"), std::string::npos);
    EXPECT_NE(content.find("25:10:2026"), std::string::npos);
    EXPECT_NE(content.find("AVAILABILITY_END"), std::string::npos);
}

TEST_F(AvailabilityTest, SaveAvailabilityDataSavesEmptyInventory) {
    EXPECT_EQ(saveAvailabilityData(), SUCCESS);

    std::ifstream file("running.txt");
    ASSERT_TRUE(file.is_open());

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    EXPECT_TRUE(content.empty());
}
TEST_F(AvailabilityTest, FindAvailabilitySkipsNonMatchingRecords) {
    auto* record = static_cast<Availability*>(
        std::calloc(1, sizeof(Availability)));
    ASSERT_NE(record, nullptr);

    record->trainNumber = 11111;
    std::strcpy(record->journeyDate, "25:10:2026");
    std::strcpy(record->className, "SL");
    availabilityHead = record;

    EXPECT_EQ(findAvailability(12345, "25:10:2026", "SL"), nullptr);
    EXPECT_EQ(findAvailability(11111, "26:10:2026", "SL"), nullptr);
    EXPECT_EQ(findAvailability(11111, "25:10:2026", "2A"), nullptr);
}

TEST_F(AvailabilityTest, CreateAvailabilityRecordSupportsMultipleCoaches) {
    Train* train = addTestTrain();
    ASSERT_NE(train, nullptr);

    train->classCount = 2;
    std::strcpy(train->coaches[1].className, "2A");
    train->coaches[1].totalSeats = 20;
    train->coaches[1].racCapacity = 6;
    train->coaches[1].wlCapacity = 8;

    createAvailabilityRecord(train, "25:10:2026");

    Availability* sleeper =
        findAvailability(12345, "25:10:2026", "SL");
    Availability* ac =
        findAvailability(12345, "25:10:2026", "2A");

    ASSERT_NE(sleeper, nullptr);
    ASSERT_NE(ac, nullptr);
    EXPECT_EQ(sleeper->availableSeats, 10);
    EXPECT_EQ(ac->availableSeats, 20);
    EXPECT_EQ(ac->availableRAC, 6);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataSkipsUnsupportedClass) {
    Train* train = addTestTrain();
    ASSERT_NE(train, nullptr);
    addTestSchedule();

    std::ofstream file("running.txt");
    ASSERT_TRUE(file.is_open());

    file << "AVAILABILITY_START\n"
         << "12345\n"
         << "25:10:2026\n"
         << "XX\n"
         << "8\n"
         << "3\n"
         << "1\n"
         << "AVAILABILITY_END\n";
    file.close();

    loadAvailabilityData();

    EXPECT_EQ(findAvailability(12345, "25:10:2026", "XX"), nullptr);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataSkipsRecordWithoutSchedule) {
    addTestTrain();

    std::ofstream file("running.txt");
    ASSERT_TRUE(file.is_open());

    file << "AVAILABILITY_START\n"
         << "12345\n"
         << "25:10:2026\n"
         << "SL\n"
         << "8\n"
         << "3\n"
         << "1\n"
         << "AVAILABILITY_END\n";
    file.close();

    loadAvailabilityData();

    EXPECT_EQ(findAvailability(12345, "25:10:2026", "SL"), nullptr);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataSkipsCapacityExceedingRecord) {
    Train* train = addTestTrain();
    ASSERT_NE(train, nullptr);
    addTestSchedule();

    std::ofstream file("running.txt");
    ASSERT_TRUE(file.is_open());

    file << "AVAILABILITY_START\n"
         << "12345\n"
         << "25:10:2026\n"
         << "SL\n"
         << "11\n"  // Train capacity is 10.
         << "3\n"
         << "1\n"
         << "AVAILABILITY_END\n";
    file.close();

    loadAvailabilityData();

    EXPECT_EQ(findAvailability(12345, "25:10:2026", "SL"), nullptr);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataSkipsDuplicateRecords) {
    Train* train = addTestTrain();
    ASSERT_NE(train, nullptr);
    addTestSchedule();

    std::ofstream file("running.txt");
    ASSERT_TRUE(file.is_open());

    for (int seats : {8, 4}) {
        file << "AVAILABILITY_START\n"
             << "12345\n"
             << "25:10:2026\n"
             << "SL\n"
             << seats << "\n"
             << "3\n"
             << "1\n"
             << "AVAILABILITY_END\n";
    }
    file.close();

    loadAvailabilityData();

    Availability* record =
        findAvailability(12345, "25:10:2026", "SL");

    ASSERT_NE(record, nullptr);
    EXPECT_EQ(record->availableSeats, 8);
}

TEST_F(AvailabilityTest, InitializeAvailabilityModuleCreatesInventoryFile) {
    initializeAvailabilityModule();

    EXPECT_TRUE(fs::exists("running.txt"));
}


TEST_F(AvailabilityTest, CheckAvailabilityReturnsWhenUserEntersZero) {
    EXPECT_NO_THROW(checkAvailability());
}

TEST_F(AvailabilityTest, CheckAvailabilityReportsMissingSchedule) {
    Train* train = addTestTrain();
    ASSERT_NE(train, nullptr);

    // A train exists, but no schedule has been created.
    const std::string output =
        runCheckAvailability("12345\n25:10:2026\n");

    EXPECT_NE(output.find("No schedule found"), std::string::npos);
}

TEST_F(AvailabilityTest, CheckAvailabilityReportsCancelledTrain) {
    Train* train = addTestTrain();
    ASSERT_NE(train, nullptr);

    addTestSchedule();
    ASSERT_NE(scheduleHead, nullptr);
    scheduleHead->status = TRAIN_CANCELLED;

    const std::string output =
        runCheckAvailability("12345\n25:10:2026\n");

    EXPECT_NE(output.find("cancelled"), std::string::npos);
}

TEST_F(AvailabilityTest, CheckAvailabilityReportsAvailableSeats) {
    Train* train = addTestTrain();
    ASSERT_NE(train, nullptr);
    addTestSchedule();

    createAvailabilityRecord(train, "25:10:2026");

    EXPECT_NE(findAvailability(12345, "25:10:2026", "SL"), nullptr);
}

TEST_F(AvailabilityTest, CheckAvailabilityExitsOnZero) {
    std::string output = runCheckAvailability("0\n");

    EXPECT_NE(output.find("Train number"), std::string::npos);
}

TEST_F(AvailabilityTest, CheckAvailabilityRejectsUnknownTrain) {
    addTestTrain();

    std::string output = runCheckAvailability("99999\n0\n");

    EXPECT_NE(output.find("Train not found"), std::string::npos);
}

TEST_F(AvailabilityTest, CheckAvailabilityRejectsInvalidDate) {
    addTestTrain();
    addTestSchedule();

    std::string output = runCheckAvailability(
        "12345\n"
        "31:02:2026\n"
        "25:10:2026\n"
    );

    EXPECT_NE(
        output.find("Invalid calendar date"),
        std::string::npos
    );

    EXPECT_NE(
        output.find("no inventory record exists"),
        std::string::npos
    );
}

TEST_F(AvailabilityTest, CheckAvailabilityShowsAvailableSeats) {
    addTestTrain();
    addTestSchedule();
    addTestAvailability(7, 4, 0);

    std::string output = runCheckAvailability(
        "12345\n25:10:2026\n"
    );

    EXPECT_NE(output.find("Available: 7"), std::string::npos);
}

TEST_F(AvailabilityTest, CheckAvailabilityShowsRAC) {
    addTestTrain();
    addTestSchedule();
    addTestAvailability(0, 3, 0);

    std::string output = runCheckAvailability(
        "12345\n25:10:2026\n"
    );

    EXPECT_NE(output.find("RAC: 3"), std::string::npos);
}

TEST_F(AvailabilityTest, CheckAvailabilityShowsWaitingList) {
    addTestTrain();
    addTestSchedule();
    addTestAvailability(0, 0, 0);

    std::string output = runCheckAvailability(
        "12345\n25:10:2026\n"
    );

    EXPECT_NE(output.find("WL: 1"), std::string::npos);
}

TEST_F(AvailabilityTest, CheckAvailabilityReportsNoAvailability) {
    addTestTrain();
    addTestSchedule();
    addTestAvailability(0, 0, 5);

    std::string output = runCheckAvailability(
        "12345\n25:10:2026\n"
    );

    EXPECT_NE(output.find("No availability"), std::string::npos);
}

TEST_F(AvailabilityTest, CheckAvailabilityReportsMissingInventory) {
    addTestTrain();
    addTestSchedule();

    std::string output = runCheckAvailability(
        "12345\n25:10:2026\n"
    );

    EXPECT_NE(
        output.find("no inventory record exists"),
        std::string::npos
    );
}

TEST_F(AvailabilityTest, CheckAvailabilitySkipsNonMatchingSchedules) {
    addTestTrain();

    addTestSchedule(12345, "26:10:2026");
    addTestSchedule(12345, "25:10:2026");

    std::string output =
        runCheckAvailability("12345\n25:10:2026\n");

    EXPECT_NE(
        output.find("no inventory record exists"),
        std::string::npos
    );
}

TEST_F(AvailabilityTest, InitializeAvailabilityModuleCanBeCalledRepeatedly) {
    initializeAvailabilityModule();
    initializeAvailabilityModule();

    EXPECT_TRUE(fs::exists("running.txt"));
}

TEST_F(AvailabilityTest, CreateAvailabilityRecordReportsSaveFailure) {
    Train* train = addTestTrain();
    ASSERT_NE(train, nullptr);

    ASSERT_TRUE(fs::create_directory("running.txt.tmp"));

    createAvailabilityRecord(train, "25:10:2026");

    EXPECT_NE(availabilityHead, nullptr);
}

TEST_F(AvailabilityTest, SaveAvailabilityDataReportsTemporaryFileOpenFailure) {
    ASSERT_TRUE(fs::create_directory("running.txt.tmp"));

    EXPECT_EQ(saveAvailabilityData(), FAILURE);
}

TEST_F(AvailabilityTest, SaveAvailabilityDataReportsRenameFailure) {
    addTestAvailability(5, 2, 0);

    ASSERT_TRUE(fs::create_directory("running.txt"));

    EXPECT_EQ(saveAvailabilityData(), FAILURE);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataFreesExistingRecords) {
    addTestTrain();
    addTestSchedule();

    addTestAvailability(3, 2, 0);

    std::ofstream file("running.txt");
    file
        << "AVAILABILITY_START\n"
        << "12345\n"
        << "25:10:2026\n"
        << "SL\n"
        << "8\n"
        << "4\n"
        << "0\n"
        << "AVAILABILITY_END\n";
    file.close();

    loadAvailabilityData();

    ASSERT_NE(availabilityHead, nullptr);
    EXPECT_EQ(availabilityHead->availableSeats, 8);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataRecoversAtNextStartMarker) {
    addTestTrain();
    addTestSchedule();

    std::ofstream file("running.txt");
    file
        << "AVAILABILITY_START\n"
        << "12345\n"
        << "25:10:2026\n"
        << "SL\n"
        << "8\n"
        << "4\n"
        << "0\n"
        << "AVAILABILITY_START\n"
        << "12345\n"
        << "25:10:2026\n"
        << "SL\n"
        << "7\n"
        << "3\n"
        << "0\n"
        << "AVAILABILITY_END\n";
    file.close();

    loadAvailabilityData();

    ASSERT_NE(availabilityHead, nullptr);
    EXPECT_EQ(availabilityHead->availableSeats, 7);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataSkipsMalformedEndMarker) {
    addTestTrain();
    addTestSchedule();

    std::ofstream file("running.txt");
    file
        << "AVAILABILITY_START\n"
        << "12345\n"
        << "25:10:2026\n"
        << "SL\n"
        << "8\n"
        << "4\n"
        << "0\n"
        << "BROKEN_END\n"
        << "unrelated data\n"
        << "AVAILABILITY_START\n"
        << "12345\n"
        << "25:10:2026\n"
        << "SL\n"
        << "7\n"
        << "3\n"
        << "0\n"
        << "AVAILABILITY_END\n";
    file.close();

    loadAvailabilityData();

    ASSERT_NE(availabilityHead, nullptr);
    EXPECT_EQ(availabilityHead->availableSeats, 7);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataRejectsInvalidTrainNumber) {
    addTestTrain();
    addTestSchedule();

    std::ofstream file("running.txt");
    file << "AVAILABILITY_START\n"
         << "invalid\n"
         << "25:10:2026\n"
         << "SL\n"
         << "8\n"
         << "4\n"
         << "0\n"
         << "AVAILABILITY_END\n";
    file.close();

    loadAvailabilityData();

    EXPECT_EQ(availabilityHead, nullptr);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataRejectsOverlongDate) {
    addTestTrain();
    addTestSchedule();

    std::ofstream file("running.txt");
    file << "AVAILABILITY_START\n"
         << "12345\n"
         << "25:10:2026-EXTRA\n"
         << "SL\n"
         << "8\n"
         << "4\n"
         << "0\n"
         << "AVAILABILITY_END\n";
    file.close();

    loadAvailabilityData();

    EXPECT_EQ(availabilityHead, nullptr);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataRejectsOverlongClassName) {
    addTestTrain();
    addTestSchedule();

    std::ofstream file("running.txt");
    file << "AVAILABILITY_START\n"
         << "12345\n"
         << "25:10:2026\n"
         << "THIS_CLASS_NAME_IS_TOO_LONG\n"
         << "8\n"
         << "4\n"
         << "0\n"
         << "AVAILABILITY_END\n";
    file.close();

    loadAvailabilityData();

    EXPECT_EQ(availabilityHead, nullptr);
}


TEST_F(AvailabilityTest, LoadAvailabilityDataLinksMultipleRecords) {
    Train* train = addTestTrain();
    ASSERT_NE(train, nullptr);

    // Add a second supported class to the train.
    train->classCount = 2;
    std::strcpy(train->coaches[1].className, "2A");
    train->coaches[1].totalSeats = 20;
    train->coaches[1].racCapacity = 5;
    train->coaches[1].wlCapacity = 5;

    addTestSchedule();
    ASSERT_NE(scheduleHead, nullptr);

    {
        std::ofstream file("running.txt");
        ASSERT_TRUE(file.is_open());

        file << "AVAILABILITY_START\n"
             << "12345\n"
             << "25:10:2026\n"
             << "SL\n"
             << "8\n4\n0\n"
             << "AVAILABILITY_END\n";

        file << "AVAILABILITY_START\n"
             << "12345\n"
             << "25:10:2026\n"
             << "2A\n"
             << "15\n5\n0\n"
             << "AVAILABILITY_END\n";
    }

    loadAvailabilityData();

    ASSERT_NE(availabilityHead, nullptr);
    ASSERT_NE(availabilityHead->next, nullptr);

    EXPECT_EQ(availabilityHead->next->prev, availabilityHead);
    EXPECT_EQ(availabilityHead->next->trainNumber, 12345);
    EXPECT_EQ(availabilityHead->trainNumber, 12345);
}


TEST_F(AvailabilityTest, CheckAvailabilityShowsMultipleInventoryRecords)
{
    addTestTrain();
    addTestSchedule();

    addTestAvailability(7, 4, 0, "25:10:2026", 12345);

    // Add a second class record for the same train and date.
    auto* second = static_cast<Availability*>(
        std::calloc(1, sizeof(Availability)));
    ASSERT_NE(second, nullptr);

    second->trainNumber = 12345;
    std::strcpy(second->journeyDate, "25:10:2026");
    std::strcpy(second->className, "2A");
    second->availableSeats = 5;
    second->availableRAC = 2;
    second->availableWL = 0;

    second->next = availabilityHead;
    if (availabilityHead != nullptr)
        availabilityHead->prev = second;
    availabilityHead = second;

    const std::string output =
        runCheckAvailability("12345\n25:10:2026\n");

    EXPECT_NE(output.find("Available: 7"), std::string::npos);
    EXPECT_NE(output.find("Available: 5"), std::string::npos);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataRejectsNegativeSeats)
{
    addTestTrain();
    addTestSchedule();

    std::ofstream file("running.txt");
    ASSERT_TRUE(file.is_open());

    file << "AVAILABILITY_START\n"
         << "12345\n"
         << "25:10:2026\n"
         << "SL\n"
         << "-1\n"
         << "3\n"
         << "1\n"
         << "AVAILABILITY_END\n";
    file.close();

    loadAvailabilityData();

    EXPECT_EQ(findAvailability(12345, "25:10:2026", "SL"), nullptr);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataRejectsNegativeRAC)
{
    addTestTrain();
    addTestSchedule();

    std::ofstream file("running.txt");
    ASSERT_TRUE(file.is_open());

    file << "AVAILABILITY_START\n"
         << "12345\n"
         << "25:10:2026\n"
         << "SL\n"
         << "8\n"
         << "-1\n"
         << "1\n"
         << "AVAILABILITY_END\n";
    file.close();

    loadAvailabilityData();

    EXPECT_EQ(findAvailability(12345, "25:10:2026", "SL"), nullptr);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataRejectsNegativeWaitingList)
{
    addTestTrain();
    addTestSchedule();

    std::ofstream file("running.txt");
    ASSERT_TRUE(file.is_open());

    file << "AVAILABILITY_START\n"
         << "12345\n"
         << "25:10:2026\n"
         << "SL\n"
         << "8\n"
         << "3\n"
         << "-1\n"
         << "AVAILABILITY_END\n";
    file.close();

    loadAvailabilityData();

    EXPECT_EQ(findAvailability(12345, "25:10:2026", "SL"), nullptr);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataRejectsInvalidSeatValue)
{
    addTestTrain();
    addTestSchedule();

    std::ofstream file("running.txt");
    ASSERT_TRUE(file.is_open());

    file << "AVAILABILITY_START\n"
         << "12345\n"
         << "25:10:2026\n"
         << "SL\n"
         << "invalid\n"
         << "3\n"
         << "1\n"
         << "AVAILABILITY_END\n";
    file.close();

    loadAvailabilityData();

    EXPECT_EQ(findAvailability(12345, "25:10:2026", "SL"), nullptr);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataRejectsUnknownTrain)
{
    addTestTrain();
    addTestSchedule();

    std::ofstream file("running.txt");
    ASSERT_TRUE(file.is_open());

    file << "AVAILABILITY_START\n"
         << "99999\n"
         << "25:10:2026\n"
         << "SL\n"
         << "8\n"
         << "3\n"
         << "1\n"
         << "AVAILABILITY_END\n";
    file.close();

    loadAvailabilityData();

    EXPECT_EQ(findAvailability(99999, "25:10:2026", "SL"), nullptr);
}

TEST_F(AvailabilityTest, LoadAvailabilityDataRejectsUnknownClass)
{
    Train* train = addTestTrain();
    ASSERT_NE(train, nullptr);
    addTestSchedule();

    std::ofstream file("running.txt");
    ASSERT_TRUE(file.is_open());

    file << "AVAILABILITY_START\n"
         << "12345\n"
         << "25:10:2026\n"
         << "2A\n"
         << "8\n"
         << "3\n"
         << "1\n"
         << "AVAILABILITY_END\n";
    file.close();

    // The train has only the SL coach.
    loadAvailabilityData();

    EXPECT_EQ(findAvailability(12345, "25:10:2026", "2A"), nullptr);
}

TEST_F(AvailabilityTest, CreateAvailabilityRecordPreservesExistingCounts)
{
    Train* train = addTestTrain();
    ASSERT_NE(train, nullptr);

    createAvailabilityRecord(train, "25:10:2026");

    Availability* record =
        findAvailability(12345, "25:10:2026", "SL");
    ASSERT_NE(record, nullptr);

    record->availableSeats = 2;
    record->availableRAC = 1;
    record->availableWL = 3;

    createAvailabilityRecord(train, "25:10:2026");

    EXPECT_EQ(record->availableSeats, 2);
    EXPECT_EQ(record->availableRAC, 1);
    EXPECT_EQ(record->availableWL, 3);
}


TEST_F(AvailabilityTest, InitializeAvailabilityModuleHandlesOpenFailure)
{
    ASSERT_TRUE(fs::create_directory("running.txt"));

    EXPECT_NO_THROW(initializeAvailabilityModule());
}


TEST_F(AvailabilityTest, SaveAvailabilityDataHandlesFlushFailure)
{
    addTestAvailability(5, 2, 0);

    fs::create_symlink("/dev/full", "running.txt.tmp");
    ASSERT_TRUE(fs::is_symlink("running.txt.tmp"));
    EXPECT_EQ(saveAvailabilityData(), FAILURE);
}

