#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

extern "C" {
#include "passenger.h"

int getIntegerInput(const char *message);
void getStringInput(const char *message, char *buffer, int size);
}

namespace {

std::vector<int> integerInputs;
std::vector<std::string> stringInputs;

size_t integerIndex = 0;
size_t stringIndex = 0;

void clearPassengers()
{
    Passenger *current = passengerHead;

    while (current != nullptr) {
        Passenger *next = current->next;
        free(current);
        current = next;
    }

    passengerHead = nullptr;
}

void resetInputs()
{
    integerInputs.clear();
    stringInputs.clear();

    integerIndex = 0;
    stringIndex = 0;
}

Passenger *addPassenger(
    int id,
    const char *name,
    int age,
    const char *gender,
    const char *phone)
{
    Passenger *p = static_cast<Passenger *>(malloc(sizeof(Passenger)));

    if (p == nullptr) {
        return nullptr;
    }

    p->passengerId = id;

    snprintf(
        p->passengerName,
        sizeof(p->passengerName),
        "%s",
        name);

    p->age = age;

    snprintf(
        p->gender,
        sizeof(p->gender),
        "%s",
        gender);

    snprintf(
        p->phoneNumber,
        sizeof(p->phoneNumber),
        "%s",
        phone);

    p->prev = nullptr;
    p->next = passengerHead;

    if (passengerHead != nullptr) {
        passengerHead->prev = p;
    }

    passengerHead = p;

    return p;
}

class PassengerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        clearPassengers();
        resetInputs();
        std::remove("passengers.txt");
    }

    void TearDown() override
    {
        clearPassengers();
        resetInputs();
        std::remove("passengers.txt");
    }
};

} // namespace

// Stub the input functions used by passenger.c.
extern "C" {

int getIntegerInput(const char *message)
{
    (void)message;

    if (integerIndex >= integerInputs.size()) {
        return 0;
    }

    return integerInputs[integerIndex++];
}

void getStringInput(const char *message, char *buffer, int size)
{
    (void)message;

    if (size <= 0) {
        return;
    }

    if (stringIndex >= stringInputs.size()) {
        buffer[0] = '\0';
        return;
    }

    const std::string &value = stringInputs[stringIndex++];

    std::snprintf(
        buffer,
        static_cast<size_t>(size),
        "%s",
        value.c_str());
}

} // extern "C"

// --------------------------------------------------
// PROFILE VIEW TESTS
// --------------------------------------------------

TEST_F(PassengerTest, ViewMyProfileWhenPassengerExists)
{
    addPassenger(101, "Arun Kumar", 25, "Male", "9876543210");

    testing::internal::CaptureStdout();
    viewMyProfile(101);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Arun Kumar"), std::string::npos);
    EXPECT_NE(output.find("9876543210"), std::string::npos);
    EXPECT_NE(output.find("Passenger ID : 101"), std::string::npos);
}

TEST_F(PassengerTest, ViewMyProfileWhenPassengerDoesNotExist)
{
    testing::internal::CaptureStdout();
    viewMyProfile(999);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Profile Not Found"), std::string::npos);
}

// --------------------------------------------------
// PASSENGER CREATION TESTS
// --------------------------------------------------

TEST_F(PassengerTest, CreatePassengerProfileSuccessfully)
{
    stringInputs = {
        "Arun Kumar",
        "M",
        "9876543210"
    };

    integerInputs = {25};

    testing::internal::CaptureStdout();
    createPassengerProfile(101);
    std::string output = testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_EQ(passengerHead->passengerId, 101);
    EXPECT_STREQ(passengerHead->passengerName, "Arun Kumar");
    EXPECT_EQ(passengerHead->age, 25);
    EXPECT_STREQ(passengerHead->gender, "Male");
    EXPECT_STREQ(passengerHead->phoneNumber, "9876543210");
    EXPECT_NE(output.find("Passenger Created Successfully"), std::string::npos);
}

TEST_F(PassengerTest, CreatePassengerProfileRejectsDuplicateId)
{
    addPassenger(101, "Arun Kumar", 25, "Male", "9876543210");

    createPassengerProfile(101);

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_EQ(passengerHead->passengerId, 101);
    EXPECT_EQ(passengerHead->next, nullptr);
}

TEST_F(PassengerTest, CreatePassengerProfileRejectsInvalidNameAndAge)
{
    stringInputs = {
        "Arun123",
        "Arun Kumar",
        "M",
        "9876543210"
    };

    integerInputs = {0, 25};

    testing::internal::CaptureStdout();
    createPassengerProfile(102);
    std::string output = testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_STREQ(passengerHead->passengerName, "Arun Kumar");
    EXPECT_EQ(passengerHead->age, 25);
    EXPECT_NE(output.find("Invalid Name"), std::string::npos);
    EXPECT_NE(output.find("Invalid Age"), std::string::npos);
}

TEST_F(PassengerTest, CreatePassengerProfileRejectsInvalidGenderAndPhone)
{
    stringInputs = {
        "Meena",
        "X",
        "F",
        "1234567890",
        "9876543210"
    };

    integerInputs = {30};

    testing::internal::CaptureStdout();
    createPassengerProfile(103);
    std::string output = testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_STREQ(passengerHead->gender, "Female");
    EXPECT_STREQ(passengerHead->phoneNumber, "9876543210");
    EXPECT_NE(output.find("Invalid Gender"), std::string::npos);
    EXPECT_NE(output.find("Invalid Phone Number"), std::string::npos);
}

// --------------------------------------------------
// VIEW ALL PASSENGERS TESTS
// --------------------------------------------------

TEST_F(PassengerTest, ViewPassengersWhenListIsEmpty)
{
    testing::internal::CaptureStdout();
    viewPassengers();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("No Passenger Available"), std::string::npos);
}

TEST_F(PassengerTest, ViewPassengersDisplaysAllPassengers)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");
    addPassenger(102, "Meena", 30, "Female", "9123456780");

    testing::internal::CaptureStdout();
    viewPassengers();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Arun"), std::string::npos);
    EXPECT_NE(output.find("Meena"), std::string::npos);
}

// --------------------------------------------------
// SEARCH TESTS
// --------------------------------------------------

TEST_F(PassengerTest, SearchPassengerByExistingId)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");

    integerInputs = {1, 101};

    testing::internal::CaptureStdout();
    searchPassenger();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Passenger Found"), std::string::npos);
    EXPECT_NE(output.find("Arun"), std::string::npos);
}

TEST_F(PassengerTest, SearchPassengerByMissingId)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");

    integerInputs = {1, 999};

    testing::internal::CaptureStdout();
    searchPassenger();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Passenger Not Found"), std::string::npos);
}

TEST_F(PassengerTest, SearchPassengerByPhoneNumber)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");

    integerInputs = {2};
    stringInputs = {"9876543210"};

    testing::internal::CaptureStdout();
    searchPassenger();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Passenger Found"), std::string::npos);
    EXPECT_NE(output.find("Arun"), std::string::npos);
}

TEST_F(PassengerTest, SearchPassengerWithInvalidChoice)
{
    integerInputs = {9};

    testing::internal::CaptureStdout();
    searchPassenger();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Invalid Choice"), std::string::npos);
}

// --------------------------------------------------
// UPDATE PROFILE TESTS
// --------------------------------------------------

TEST_F(PassengerTest, UpdatePassengerName)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");

    integerInputs = {1};
    stringInputs = {"Arun Kumar"};

    updateMyProfile(101);

    EXPECT_STREQ(passengerHead->passengerName, "Arun Kumar");
}

TEST_F(PassengerTest, UpdatePassengerAge)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");

    integerInputs = {2, 0, 35};

    testing::internal::CaptureStdout();
    updateMyProfile(101);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(passengerHead->age, 35);
    EXPECT_NE(output.find("Invalid Age"), std::string::npos);
}

TEST_F(PassengerTest, UpdatePassengerPhoneNumber)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");

    integerInputs = {3};
    stringInputs = {"1234567890", "9123456780"};

    testing::internal::CaptureStdout();
    updateMyProfile(101);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_STREQ(passengerHead->phoneNumber, "9123456780");
    EXPECT_NE(output.find("Invalid Phone Number"), std::string::npos);
}

TEST_F(PassengerTest, UpdatePassengerWithInvalidChoice)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");

    integerInputs = {9};

    testing::internal::CaptureStdout();
    updateMyProfile(101);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Invalid Choice"), std::string::npos);
}

TEST_F(PassengerTest, UpdateMissingPassenger)
{
    testing::internal::CaptureStdout();
    updateMyProfile(999);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Passenger Not Found"), std::string::npos);
}

// --------------------------------------------------
// DELETE PASSENGER TESTS
// --------------------------------------------------

TEST_F(PassengerTest, DeletePassengerWithConfirmation)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");

    integerInputs = {101};
    stringInputs = {"Y"};

    testing::internal::CaptureStdout();
    deletePassenger();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(passengerHead, nullptr);
    EXPECT_NE(output.find("Passenger Deleted Successfully"), std::string::npos);
}

TEST_F(PassengerTest, CancelPassengerDeletion)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");

    integerInputs = {101};
    stringInputs = {"N"};

    testing::internal::CaptureStdout();
    deletePassenger();
    std::string output = testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_EQ(passengerHead->passengerId, 101);
    EXPECT_NE(output.find("Deletion aborted"), std::string::npos);
}

TEST_F(PassengerTest, DeleteMissingPassenger)
{
    integerInputs = {999};

    testing::internal::CaptureStdout();
    deletePassenger();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Passenger Not Found"), std::string::npos);
}

// --------------------------------------------------
// SAVE AND LOAD TESTS
// --------------------------------------------------

TEST_F(PassengerTest, SavePassengerDataCreatesFile)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");

    savePassengerData();

    std::ifstream file("passengers.txt");
    ASSERT_TRUE(file.is_open());

    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    EXPECT_NE(content.find("PASSENGER_START"), std::string::npos);
    EXPECT_NE(content.find("Arun"), std::string::npos);
    EXPECT_NE(content.find("PASSENGER_END"), std::string::npos);
}

TEST_F(PassengerTest, LoadPassengerDataLoadsValidRecord)
{
    {
        std::ofstream file("passengers.txt");

        file << "PASSENGER_START\n"
             << "101\n"
             << "Arun Kumar\n"
             << "25\n"
             << "Male\n"
             << "9876543210\n"
             << "PASSENGER_END\n";
    }

    loadPassengerData();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_EQ(passengerHead->passengerId, 101);
    EXPECT_STREQ(passengerHead->passengerName, "Arun Kumar");
    EXPECT_EQ(passengerHead->age, 25);
    EXPECT_STREQ(passengerHead->gender, "Male");
    EXPECT_STREQ(passengerHead->phoneNumber, "9876543210");
}

TEST_F(PassengerTest, LoadPassengerDataSkipsInvalidRecord)
{
    {
        std::ofstream file("passengers.txt");

        file << "PASSENGER_START\n"
             << "101\n"
             << "Arun123\n"
             << "25\n"
             << "Male\n"
             << "9876543210\n"
             << "PASSENGER_END\n";
    }

    testing::internal::CaptureStdout();
    loadPassengerData();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(passengerHead, nullptr);
    EXPECT_NE(output.find("Invalid passenger record skipped"), std::string::npos);
}

TEST_F(PassengerTest, InitializePassengerModuleCreatesFile)
{
    initializePassengerModule();

    std::ifstream file("passengers.txt");
    EXPECT_TRUE(file.is_open());
}

TEST_F(PassengerTest, ViewMyProfileFindsPassengerAfterHead)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");
    addPassenger(102, "Meena", 30, "Female", "9123456780");

    testing::internal::CaptureStdout();
    viewMyProfile(101);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Arun"), std::string::npos);
}

TEST_F(PassengerTest, CreatePassengerProfileWithOtherGender)
{
    stringInputs = {"Kavin", "O", "9123456780"};
    integerInputs = {20};

    testing::internal::CaptureStdout();
    createPassengerProfile(104);
    testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_STREQ(passengerHead->gender, "Other");
}

TEST_F(PassengerTest, CreatePassengerProfileWithMinimumValidAge)
{
    stringInputs = {"Kavin", "M", "9123456780"};
    integerInputs = {1};

    testing::internal::CaptureStdout();
    createPassengerProfile(105);
    testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_EQ(passengerHead->age, 1);
}

TEST_F(PassengerTest, CreatePassengerProfileWithMaximumValidAge)
{
    stringInputs = {"Kavin", "F", "9123456780"};
    integerInputs = {120};

    testing::internal::CaptureStdout();
    createPassengerProfile(106);
    testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_EQ(passengerHead->age, 120);
}

TEST_F(PassengerTest, DeletePassengerFromHeadOfMultipleNodeList)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");
    addPassenger(102, "Meena", 30, "Female", "9123456780");
    addPassenger(103, "Kavin", 22, "Male", "9876501234");

    // addPassenger inserts at the head, so 103 is currently the head.
    integerInputs = {103};
    stringInputs = {"Y"};

    testing::internal::CaptureStdout();
    deletePassenger();
    testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_EQ(passengerHead->passengerId, 102);
    EXPECT_EQ(passengerHead->prev, nullptr);
    ASSERT_NE(passengerHead->next, nullptr);
    EXPECT_EQ(passengerHead->next->passengerId, 101);
    EXPECT_EQ(passengerHead->next->prev, passengerHead);
}

TEST_F(PassengerTest, DeletePassengerFromMiddleOfList)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");
    addPassenger(102, "Meena", 30, "Female", "9123456780");
    addPassenger(103, "Kavin", 22, "Male", "9876501234");

    integerInputs = {102};
    stringInputs = {"Y"};

    testing::internal::CaptureStdout();
    deletePassenger();
    testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_EQ(passengerHead->passengerId, 103);

    ASSERT_NE(passengerHead->next, nullptr);
    EXPECT_EQ(passengerHead->next->passengerId, 101);
    EXPECT_EQ(passengerHead->next->prev, passengerHead);
    EXPECT_EQ(passengerHead->next->next, nullptr);
}

TEST_F(PassengerTest, DeletePassengerFromTailOfList)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");
    addPassenger(102, "Meena", 30, "Female", "9123456780");
    addPassenger(103, "Kavin", 22, "Male", "9876501234");

    integerInputs = {101};
    stringInputs = {"Y"};

    testing::internal::CaptureStdout();
    deletePassenger();
    testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_EQ(passengerHead->passengerId, 103);

    ASSERT_NE(passengerHead->next, nullptr);
    EXPECT_EQ(passengerHead->next->passengerId, 102);
    EXPECT_EQ(passengerHead->next->next, nullptr);
}

TEST_F(PassengerTest, SearchPassengerByMissingPhoneNumber)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");

    integerInputs = {2};
    stringInputs = {"9000000000"};

    testing::internal::CaptureStdout();
    searchPassenger();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Passenger Not Found"), std::string::npos);
}

TEST_F(PassengerTest, LoadPassengerDataSkipsDuplicateIds)
{
    {
        std::ofstream file("passengers.txt");

        file << "PASSENGER_START\n"
             << "101\n"
             << "Arun\n"
             << "25\n"
             << "Male\n"
             << "9876543210\n"
             << "PASSENGER_END\n";

        file << "PASSENGER_START\n"
             << "101\n"
             << "Meena\n"
             << "30\n"
             << "Female\n"
             << "9123456780\n"
             << "PASSENGER_END\n";
    }

    testing::internal::CaptureStdout();
    loadPassengerData();
    testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_EQ(passengerHead->passengerId, 101);
    EXPECT_EQ(passengerHead->next, nullptr);
}

// ==================================================
// ADDITIONAL TESTS FOR UNCOVERED PASSENGER PATHS
// ==================================================

#include <sys/stat.h>
#include <unistd.h>

// --------------------------------------------------
// 1. EMPTY NAME VALIDATION
// Covers validateName() empty-string rejection.
// --------------------------------------------------

TEST_F(PassengerTest, RejectsEmptyNameThenAcceptsValidName)
{
    stringInputs = {
        "",
        "Arun Kumar",
        "M",
        "9876543210"
    };

    integerInputs = {25};

    testing::internal::CaptureStdout();
    createPassengerProfile(201);
    std::string output = testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_STREQ(passengerHead->passengerName, "Arun Kumar");
    EXPECT_NE(output.find("Invalid Name"), std::string::npos);
}

// --------------------------------------------------
// 2. PHONE NUMBER VALIDATION
// Covers invalid length, invalid first digit,
// and nondigit characters.
// --------------------------------------------------

TEST_F(PassengerTest, RejectsPhoneNumberWithInvalidLength)
{
    stringInputs = {
        "Arun",
        "M",
        "98765",
        "9876543210"
    };

    integerInputs = {25};

    testing::internal::CaptureStdout();
    createPassengerProfile(202);
    std::string output = testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_STREQ(passengerHead->phoneNumber, "9876543210");
    EXPECT_NE(output.find("Invalid Phone Number"), std::string::npos);
}

TEST_F(PassengerTest, RejectsPhoneNumberStartingWithFive)
{
    stringInputs = {
        "Arun",
        "M",
        "5123456789",
        "9876543210"
    };

    integerInputs = {25};

    testing::internal::CaptureStdout();
    createPassengerProfile(203);
    std::string output = testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_STREQ(passengerHead->phoneNumber, "9876543210");
    EXPECT_NE(output.find("Invalid Phone Number"), std::string::npos);
}

TEST_F(PassengerTest, RejectsNondigitPhoneNumber)
{
    stringInputs = {
        "Arun",
        "M",
        "98A6543210",
        "9876543210"
    };

    integerInputs = {25};

    testing::internal::CaptureStdout();
    createPassengerProfile(204);
    std::string output = testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_STREQ(passengerHead->phoneNumber, "9876543210");
    EXPECT_NE(output.find("Invalid Phone Number"), std::string::npos);
}

// --------------------------------------------------
// 3. PASSENGER ID SEARCH THROUGH MULTIPLE NODES
// Covers traversal in passengerIdExists().
// --------------------------------------------------

TEST_F(PassengerTest, CreatePassengerAfterExistingPassengers)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");
    addPassenger(102, "Meena", 30, "Female", "9123456780");

    stringInputs = {
        "Kavin",
        "M",
        "9876501234"
    };

    integerInputs = {22};

    testing::internal::CaptureStdout();
    createPassengerProfile(103);
    testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_EQ(passengerHead->passengerId, 103);
    ASSERT_NE(passengerHead->next, nullptr);
    EXPECT_EQ(passengerHead->next->passengerId, 102);
    ASSERT_NE(passengerHead->next->next, nullptr);
    EXPECT_EQ(passengerHead->next->next->passengerId, 101);
}

// --------------------------------------------------
// 4. CREATE WITH NONEMPTY LIST
// Covers passengerHead != NULL during insertion.
// --------------------------------------------------

TEST_F(PassengerTest, CreatingPassengerUpdatesPreviousHeadLink)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");

    stringInputs = {
        "Meena",
        "F",
        "9123456780"
    };

    integerInputs = {30};

    testing::internal::CaptureStdout();
    createPassengerProfile(102);
    testing::internal::GetCapturedStdout();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_EQ(passengerHead->passengerId, 102);
    ASSERT_NE(passengerHead->next, nullptr);
    EXPECT_EQ(passengerHead->next->passengerId, 101);
    EXPECT_EQ(passengerHead->next->prev, passengerHead);
}

// --------------------------------------------------
// 5. SAVE FILE OPEN FAILURE
// On POSIX systems, fopen("passengers.txt", "w")
// fails when passengers.txt is a directory.
// --------------------------------------------------

TEST_F(PassengerTest, SavePassengerDataHandlesFileOpenFailure)
{
    ASSERT_EQ(mkdir("passengers.txt.tmp", 0700), 0);

    testing::internal::CaptureStdout();
    int result = savePassengerData();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(result, FAILURE);
    EXPECT_NE(
        output.find("Unable To Save Passenger Data"),
        std::string::npos
    );

    EXPECT_EQ(rmdir("passengers.txt.tmp"), 0);
}

// --------------------------------------------------
// 6. INITIALIZATION FILE OPEN FAILURE
// --------------------------------------------------

TEST_F(PassengerTest, InitializePassengerModuleHandlesFileOpenFailure)
{
    ASSERT_EQ(mkdir("passengers.txt", 0700), 0);

    testing::internal::CaptureStdout();
    initializePassengerModule();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(
        output.find("Unable To Create Passenger File"),
        std::string::npos
    );

    EXPECT_EQ(rmdir("passengers.txt"), 0);
}

// --------------------------------------------------
// 7. LOAD WHEN FILE DOES NOT EXIST
// Covers fopen(..., "r") returning NULL.
// --------------------------------------------------

TEST_F(PassengerTest, LoadPassengerDataHandlesMissingFile)
{
    std::remove("passengers.txt");

    loadPassengerData();

    EXPECT_EQ(passengerHead, nullptr);
}

// --------------------------------------------------
// 8. LOAD MULTIPLE RECORDS
// Covers linking a new record when the list is nonempty.
// --------------------------------------------------

TEST_F(PassengerTest, LoadPassengerDataLinksMultipleRecords)
{
    {
        std::ofstream file("passengers.txt");

        file << "PASSENGER_START\n"
             << "101\n"
             << "Arun\n"
             << "25\n"
             << "Male\n"
             << "9876543210\n"
             << "PASSENGER_END\n";

        file << "PASSENGER_START\n"
             << "102\n"
             << "Meena\n"
             << "30\n"
             << "Female\n"
             << "9123456780\n"
             << "PASSENGER_END\n";
    }

    loadPassengerData();

    ASSERT_NE(passengerHead, nullptr);
    EXPECT_EQ(passengerHead->passengerId, 102);

    ASSERT_NE(passengerHead->next, nullptr);
    EXPECT_EQ(passengerHead->next->passengerId, 101);

    EXPECT_EQ(passengerHead->prev, nullptr);
    EXPECT_EQ(passengerHead->next->prev, passengerHead);
}

// --------------------------------------------------
// 9. LOAD RECORD WITH MISSING TERMINATOR
// Covers the fgets() failure while reading the
// record terminator.
// --------------------------------------------------

TEST_F(PassengerTest, LoadPassengerDataSkipsIncompleteRecord)
{
    {
        std::ofstream file("passengers.txt");

        file << "PASSENGER_START\n"
             << "101\n"
             << "Arun\n"
             << "25\n"
             << "Male\n"
             << "9876543210\n";
        // Deliberately omit PASSENGER_END.
    }

    loadPassengerData();

    EXPECT_EQ(passengerHead, nullptr);
}

// --------------------------------------------------
// 10. LOAD RECORD WITH WRONG TERMINATOR
// --------------------------------------------------

TEST_F(PassengerTest, LoadPassengerDataSkipsWrongTerminator)
{
    {
        std::ofstream file("passengers.txt");

        file << "PASSENGER_START\n"
             << "101\n"
             << "Arun\n"
             << "25\n"
             << "Male\n"
             << "9876543210\n"
             << "WRONG_END\n";
    }

    testing::internal::CaptureStdout();
    loadPassengerData();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(passengerHead, nullptr);
    EXPECT_NE(
        output.find("Invalid passenger record skipped"),
        std::string::npos
    );
}

// --------------------------------------------------
// 11. LOAD RECORD WITH INVALID ID
// --------------------------------------------------

TEST_F(PassengerTest, LoadPassengerDataRejectsNonpositiveId)
{
    {
        std::ofstream file("passengers.txt");

        file << "PASSENGER_START\n"
             << "0\n"
             << "Arun\n"
             << "25\n"
             << "Male\n"
             << "9876543210\n"
             << "PASSENGER_END\n";
    }

    testing::internal::CaptureStdout();
    loadPassengerData();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(passengerHead, nullptr);
    EXPECT_NE(
        output.find("Invalid passenger record skipped"),
        std::string::npos
    );
}

// --------------------------------------------------
// 12. LOAD RECORD WITH INVALID AGE
// --------------------------------------------------

TEST_F(PassengerTest, LoadPassengerDataRejectsInvalidAge)
{
    {
        std::ofstream file("passengers.txt");

        file << "PASSENGER_START\n"
             << "101\n"
             << "Arun\n"
             << "121\n"
             << "Male\n"
             << "9876543210\n"
             << "PASSENGER_END\n";
    }

    testing::internal::CaptureStdout();
    loadPassengerData();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(passengerHead, nullptr);
    EXPECT_NE(
        output.find("Invalid passenger record skipped"),
        std::string::npos
    );
}

// --------------------------------------------------
// 13. LOAD RECORD WITH INVALID GENDER
// --------------------------------------------------

TEST_F(PassengerTest, LoadPassengerDataRejectsInvalidGender)
{
    {
        std::ofstream file("passengers.txt");

        file << "PASSENGER_START\n"
             << "101\n"
             << "Arun\n"
             << "25\n"
             << "Unknown\n"
             << "9876543210\n"
             << "PASSENGER_END\n";
    }

    testing::internal::CaptureStdout();
    loadPassengerData();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(passengerHead, nullptr);
    EXPECT_NE(
        output.find("Invalid passenger record skipped"),
        std::string::npos
    );
}

// --------------------------------------------------
// 14. LOAD RECORD WITH INVALID PHONE
// --------------------------------------------------

TEST_F(PassengerTest, LoadPassengerDataRejectsInvalidPhone)
{
    {
        std::ofstream file("passengers.txt");

        file << "PASSENGER_START\n"
             << "101\n"
             << "Arun\n"
             << "25\n"
             << "Male\n"
             << "1234567890\n"
             << "PASSENGER_END\n";
    }

    testing::internal::CaptureStdout();
    loadPassengerData();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(passengerHead, nullptr);
    EXPECT_NE(
        output.find("Invalid passenger record skipped"),
        std::string::npos
    );
}

// --------------------------------------------------
// 15. UPDATE SEARCH THROUGH LIST WITHOUT A MATCH
// Covers current = current->next in updateMyProfile().
// --------------------------------------------------

TEST_F(PassengerTest, UpdateMissingPassengerAfterTraversingList)
{
    addPassenger(101, "Arun", 25, "Male", "9876543210");
    addPassenger(102, "Meena", 30, "Female", "9123456780");

    testing::internal::CaptureStdout();
    updateMyProfile(999);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Passenger Not Found"), std::string::npos);
}