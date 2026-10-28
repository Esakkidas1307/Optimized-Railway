#include <gtest/gtest.h>

#include <vector>

#include "usermenu.h"

namespace {
std::vector<int> menuChoices;
std::size_t choiceIndex = 0;

int createProfileCalls = 0;
int viewProfileCalls = 0;
int updateProfileCalls = 0;
int availabilityCalls = 0;
int reservedBookingCalls = 0;
int unreservedBookingCalls = 0;
int viewBookingsCalls = 0;
int cancelTicketCalls = 0;

int receivedPassengerId = -1;

void resetTestState()
{
    menuChoices.clear();
    choiceIndex = 0;

    createProfileCalls = 0;
    viewProfileCalls = 0;
    updateProfileCalls = 0;
    availabilityCalls = 0;
    reservedBookingCalls = 0;
    unreservedBookingCalls = 0;
    viewBookingsCalls = 0;
    cancelTicketCalls = 0;

    receivedPassengerId = -1;
}
}  // namespace

// Test doubles for the external functions called by usermenu.c.

extern "C" int getIntegerInput(const char* message)
{
    (void)message;

    if (choiceIndex >= menuChoices.size()) {
        return 0;
    }

    return menuChoices[choiceIndex++];
}

extern "C" void createPassengerProfile(int passengerId)
{
    ++createProfileCalls;
    receivedPassengerId = passengerId;
}

extern "C" void viewMyProfile(int passengerId)
{
    ++viewProfileCalls;
    receivedPassengerId = passengerId;
}

extern "C" void updateMyProfile(int passengerId)
{
    ++updateProfileCalls;
    receivedPassengerId = passengerId;
}

extern "C" void checkAvailability(void)
{
    ++availabilityCalls;
}

extern "C" void bookTicket(int passengerId)
{
    ++reservedBookingCalls;
    receivedPassengerId = passengerId;
}

extern "C" void bookUnreservedTicket(int passengerId)
{
    ++unreservedBookingCalls;
    receivedPassengerId = passengerId;
}

extern "C" void viewMyBookings(int passengerId)
{
    ++viewBookingsCalls;
    receivedPassengerId = passengerId;
}

extern "C" void cancelTicket(int passengerId)
{
    ++cancelTicketCalls;
    receivedPassengerId = passengerId;
}

class UserMenuTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        resetTestState();
    }
};

TEST_F(UserMenuTest, ChoiceOneCreatesPassengerProfile)
{
    menuChoices = {1, 0};

    userMenu(101);

    EXPECT_EQ(createProfileCalls, 1);
    EXPECT_EQ(receivedPassengerId, 101);
}

TEST_F(UserMenuTest, ChoiceTwoViewsPassengerProfile)
{
    menuChoices = {2, 0};

    userMenu(102);

    EXPECT_EQ(viewProfileCalls, 1);
    EXPECT_EQ(receivedPassengerId, 102);
}

TEST_F(UserMenuTest, ChoiceThreeUpdatesPassengerProfile)
{
    menuChoices = {3, 0};

    userMenu(103);

    EXPECT_EQ(updateProfileCalls, 1);
    EXPECT_EQ(receivedPassengerId, 103);
}

TEST_F(UserMenuTest, ChoiceFourChecksAvailability)
{
    menuChoices = {4, 0};

    userMenu(104);

    EXPECT_EQ(availabilityCalls, 1);
}

TEST_F(UserMenuTest, ChoiceFiveBooksReservedTicket)
{
    menuChoices = {5, 0};

    userMenu(105);

    EXPECT_EQ(reservedBookingCalls, 1);
    EXPECT_EQ(receivedPassengerId, 105);
}

TEST_F(UserMenuTest, ChoiceSixBooksUnreservedTicket)
{
    menuChoices = {6, 0};

    userMenu(106);

    EXPECT_EQ(unreservedBookingCalls, 1);
    EXPECT_EQ(receivedPassengerId, 106);
}

TEST_F(UserMenuTest, ChoiceSevenViewsBookings)
{
    menuChoices = {7, 0};

    userMenu(107);

    EXPECT_EQ(viewBookingsCalls, 1);
    EXPECT_EQ(receivedPassengerId, 107);
}

TEST_F(UserMenuTest, ChoiceEightCancelsTicket)
{
    menuChoices = {8, 0};

    userMenu(108);

    EXPECT_EQ(cancelTicketCalls, 1);
    EXPECT_EQ(receivedPassengerId, 108);
}

TEST_F(UserMenuTest, ChoiceZeroLogsOut)
{
    menuChoices = {0};

    EXPECT_NO_THROW(userMenu(109));

    EXPECT_EQ(choiceIndex, 1U);
    EXPECT_EQ(createProfileCalls, 0);
    EXPECT_EQ(viewProfileCalls, 0);
    EXPECT_EQ(updateProfileCalls, 0);
    EXPECT_EQ(availabilityCalls, 0);
    EXPECT_EQ(reservedBookingCalls, 0);
    EXPECT_EQ(unreservedBookingCalls, 0);
    EXPECT_EQ(viewBookingsCalls, 0);
    EXPECT_EQ(cancelTicketCalls, 0);
}

TEST_F(UserMenuTest, InvalidChoiceReturnsToMenu)
{
    menuChoices = {9, 0};

    EXPECT_NO_THROW(userMenu(110));

    EXPECT_EQ(choiceIndex, 2U);
    EXPECT_EQ(createProfileCalls, 0);
    EXPECT_EQ(viewProfileCalls, 0);
    EXPECT_EQ(updateProfileCalls, 0);
    EXPECT_EQ(availabilityCalls, 0);
    EXPECT_EQ(reservedBookingCalls, 0);
    EXPECT_EQ(unreservedBookingCalls, 0);
    EXPECT_EQ(viewBookingsCalls, 0);
    EXPECT_EQ(cancelTicketCalls, 0);
}

TEST_F(UserMenuTest, MultipleChoicesAreHandledInSequence)
{
    menuChoices = {1, 4, 5, 7, 0};

    userMenu(200);

    EXPECT_EQ(createProfileCalls, 1);
    EXPECT_EQ(availabilityCalls, 1);
    EXPECT_EQ(reservedBookingCalls, 1);
    EXPECT_EQ(viewBookingsCalls, 1);

    EXPECT_EQ(updateProfileCalls, 0);
    EXPECT_EQ(unreservedBookingCalls, 0);
    EXPECT_EQ(cancelTicketCalls, 0);
}


TEST_F(UserMenuTest, ThreeInvalidChoicesReturnToMainMenu)
{
    menuChoices = {9, -1, 10};

    EXPECT_NO_THROW(userMenu(111));

    EXPECT_EQ(choiceIndex, 3U);
    EXPECT_EQ(createProfileCalls, 0);
    EXPECT_EQ(viewProfileCalls, 0);
    EXPECT_EQ(updateProfileCalls, 0);
    EXPECT_EQ(availabilityCalls, 0);
    EXPECT_EQ(reservedBookingCalls, 0);
    EXPECT_EQ(unreservedBookingCalls, 0);
    EXPECT_EQ(viewBookingsCalls, 0);
    EXPECT_EQ(cancelTicketCalls, 0);
}

TEST_F(UserMenuTest, ValidChoiceResetsInvalidChoiceCounter)
{
    menuChoices = {9, 1, 10, 2, 0};

    userMenu(112);

    EXPECT_EQ(choiceIndex, 5U);
    EXPECT_EQ(createProfileCalls, 1);
    EXPECT_EQ(viewProfileCalls, 1);
}

TEST_F(UserMenuTest, NegativeChoiceIsRejected)
{
    menuChoices = {-1, 0};

    userMenu(113);

    EXPECT_EQ(choiceIndex, 2U);
    EXPECT_EQ(createProfileCalls, 0);
    EXPECT_EQ(cancelTicketCalls, 0);
}

TEST_F(UserMenuTest, InvalidChoicesAfterValidChoiceReachLimit)
{
    menuChoices = {1, 9, 9, 9};

    userMenu(114);

    EXPECT_EQ(choiceIndex, 4U);
    EXPECT_EQ(createProfileCalls, 1);
    EXPECT_EQ(viewProfileCalls, 0);
}