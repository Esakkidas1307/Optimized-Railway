#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

#include "adminmenu.h"

namespace {

std::vector<int> menuChoices;
std::size_t choiceIndex = 0;

int addTrainCalls = 0;
int viewTrainCalls = 0;
int searchTrainCalls = 0;
int updateTrainCalls = 0;
int deleteTrainCalls = 0;

int viewPassengersCalls = 0;
int searchPassengerCalls = 0;
int deletePassengerCalls = 0;

int addScheduleCalls = 0;
int viewSchedulesCalls = 0;
int searchScheduleCalls = 0;
int updateScheduleCalls = 0;
int deleteScheduleCalls = 0;

int availabilityCalls = 0;

void resetTestState()
{
    menuChoices.clear();
    choiceIndex = 0;

    addTrainCalls = 0;
    viewTrainCalls = 0;
    searchTrainCalls = 0;
    updateTrainCalls = 0;
    deleteTrainCalls = 0;

    viewPassengersCalls = 0;
    searchPassengerCalls = 0;
    deletePassengerCalls = 0;

    addScheduleCalls = 0;
    viewSchedulesCalls = 0;
    searchScheduleCalls = 0;
    updateScheduleCalls = 0;
    deleteScheduleCalls = 0;

    availabilityCalls = 0;
}

}  // namespace

// Test doubles for functions called by adminmenu.c.

extern "C" int getIntegerInput(const char* message)
{
    (void)message;

    if (choiceIndex >= menuChoices.size()) {
        return 15;  // Safely exit the menu if choices run out.
    }

    return menuChoices[choiceIndex++];
}

extern "C" void addTrain(void)
{
    ++addTrainCalls;
}

extern "C" void viewTrainDetails(void)
{
    ++viewTrainCalls;
}

extern "C" void searchTrain(void)
{
    ++searchTrainCalls;
}

extern "C" void updateTrain(void)
{
    ++updateTrainCalls;
}

extern "C" void deleteTrain(void)
{
    ++deleteTrainCalls;
}

extern "C" void viewPassengers(void)
{
    ++viewPassengersCalls;
}

extern "C" void searchPassenger(void)
{
    ++searchPassengerCalls;
}

extern "C" void deletePassenger(void)
{
    ++deletePassengerCalls;
}

extern "C" void addTrainSchedule(void)
{
    ++addScheduleCalls;
}

extern "C" void viewTrainSchedules(void)
{
    ++viewSchedulesCalls;
}

extern "C" void searchTrainSchedule(void)
{
    ++searchScheduleCalls;
}

extern "C" void updateTrainSchedule(void)
{
    ++updateScheduleCalls;
}

extern "C" void deleteTrainSchedule(void)
{
    ++deleteScheduleCalls;
}

extern "C" void checkAvailability(void)
{
    ++availabilityCalls;
}

class AdminMenuTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        resetTestState();
    }
};

TEST_F(AdminMenuTest, ChoiceOneAddsTrain)
{
    menuChoices = {1, 15};

    adminMenu();

    EXPECT_EQ(addTrainCalls, 1);
}

TEST_F(AdminMenuTest, ChoiceTwoViewsTrains)
{
    menuChoices = {2, 15};

    adminMenu();

    EXPECT_EQ(viewTrainCalls, 1);
}

TEST_F(AdminMenuTest, ChoiceThreeSearchesTrain)
{
    menuChoices = {3, 15};

    adminMenu();

    EXPECT_EQ(searchTrainCalls, 1);
}

TEST_F(AdminMenuTest, ChoiceFourUpdatesTrain)
{
    menuChoices = {4, 15};

    adminMenu();

    EXPECT_EQ(updateTrainCalls, 1);
}

TEST_F(AdminMenuTest, ChoiceFiveDeletesTrain)
{
    menuChoices = {5, 15};

    adminMenu();

    EXPECT_EQ(deleteTrainCalls, 1);
}

TEST_F(AdminMenuTest, ChoiceSixViewsPassengers)
{
    menuChoices = {6, 15};

    adminMenu();

    EXPECT_EQ(viewPassengersCalls, 1);
}

TEST_F(AdminMenuTest, ChoiceSevenSearchesPassenger)
{
    menuChoices = {7, 15};

    adminMenu();

    EXPECT_EQ(searchPassengerCalls, 1);
}

TEST_F(AdminMenuTest, ChoiceEightDeletesPassenger)
{
    menuChoices = {8, 15};

    adminMenu();

    EXPECT_EQ(deletePassengerCalls, 1);
}

TEST_F(AdminMenuTest, ChoiceNineAddsTrainSchedule)
{
    menuChoices = {9, 15};

    adminMenu();

    EXPECT_EQ(addScheduleCalls, 1);
}

TEST_F(AdminMenuTest, ChoiceTenViewsTrainSchedules)
{
    menuChoices = {10, 15};

    adminMenu();

    EXPECT_EQ(viewSchedulesCalls, 1);
}

TEST_F(AdminMenuTest, ChoiceElevenSearchesTrainSchedule)
{
    menuChoices = {11, 15};

    adminMenu();

    EXPECT_EQ(searchScheduleCalls, 1);
}

TEST_F(AdminMenuTest, ChoiceTwelveUpdatesTrainSchedule)
{
    menuChoices = {12, 15};

    adminMenu();

    EXPECT_EQ(updateScheduleCalls, 1);
}

TEST_F(AdminMenuTest, ChoiceThirteenDeletesTrainSchedule)
{
    menuChoices = {13, 15};

    adminMenu();

    EXPECT_EQ(deleteScheduleCalls, 1);
}

TEST_F(AdminMenuTest, ChoiceFourteenChecksAvailability)
{
    menuChoices = {14, 15};

    adminMenu();

    EXPECT_EQ(availabilityCalls, 1);
}

TEST_F(AdminMenuTest, ChoiceFifteenLogsOut)
{
    menuChoices = {15};

    EXPECT_NO_THROW(adminMenu());

    EXPECT_EQ(choiceIndex, 1U);

    EXPECT_EQ(addTrainCalls, 0);
    EXPECT_EQ(viewTrainCalls, 0);
    EXPECT_EQ(searchTrainCalls, 0);
    EXPECT_EQ(updateTrainCalls, 0);
    EXPECT_EQ(deleteTrainCalls, 0);

    EXPECT_EQ(viewPassengersCalls, 0);
    EXPECT_EQ(searchPassengerCalls, 0);
    EXPECT_EQ(deletePassengerCalls, 0);

    EXPECT_EQ(addScheduleCalls, 0);
    EXPECT_EQ(viewSchedulesCalls, 0);
    EXPECT_EQ(searchScheduleCalls, 0);
    EXPECT_EQ(updateScheduleCalls, 0);
    EXPECT_EQ(deleteScheduleCalls, 0);

    EXPECT_EQ(availabilityCalls, 0);
}

TEST_F(AdminMenuTest, InvalidChoiceReturnsToMenu)
{
    menuChoices = {99, 15};

    EXPECT_NO_THROW(adminMenu());

    EXPECT_EQ(choiceIndex, 2U);

    EXPECT_EQ(addTrainCalls, 0);
    EXPECT_EQ(viewTrainCalls, 0);
    EXPECT_EQ(searchTrainCalls, 0);
    EXPECT_EQ(updateTrainCalls, 0);
    EXPECT_EQ(deleteTrainCalls, 0);

    EXPECT_EQ(viewPassengersCalls, 0);
    EXPECT_EQ(searchPassengerCalls, 0);
    EXPECT_EQ(deletePassengerCalls, 0);

    EXPECT_EQ(addScheduleCalls, 0);
    EXPECT_EQ(viewSchedulesCalls, 0);
    EXPECT_EQ(searchScheduleCalls, 0);
    EXPECT_EQ(updateScheduleCalls, 0);
    EXPECT_EQ(deleteScheduleCalls, 0);

    EXPECT_EQ(availabilityCalls, 0);
}

TEST_F(AdminMenuTest, MultipleChoicesAreHandledInSequence)
{
    menuChoices = {1, 6, 9, 14, 15};

    adminMenu();

    EXPECT_EQ(addTrainCalls, 1);
    EXPECT_EQ(viewPassengersCalls, 1);
    EXPECT_EQ(addScheduleCalls, 1);
    EXPECT_EQ(availabilityCalls, 1);

    EXPECT_EQ(viewTrainCalls, 0);
    EXPECT_EQ(searchTrainCalls, 0);
    EXPECT_EQ(updateTrainCalls, 0);
    EXPECT_EQ(deleteTrainCalls, 0);
    EXPECT_EQ(searchPassengerCalls, 0);
    EXPECT_EQ(deletePassengerCalls, 0);
    EXPECT_EQ(viewSchedulesCalls, 0);
    EXPECT_EQ(searchScheduleCalls, 0);
    EXPECT_EQ(updateScheduleCalls, 0);
    EXPECT_EQ(deleteScheduleCalls, 0);
}