#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

#include "common.h"

// The application entry point is renamed for the test build.
extern "C" {
int railway_app_main(void);

extern int loggedInPassengerId;

void initializeSystem(void);
void cleanupSystem(void);
void initializeTrainModule(void);
void initializeTrainScheduleModule(void);
void initializeAvailabilityModule(void);
void initializeBookingModule(void);
void initializePassengerModule(void);
void initializeUserModule(void);

int getIntegerInput(const char* message);
int registerUser(void);
int loginUser(void);
int adminLogin(void);

void userMenu(int passengerId);
void adminMenu(void);
void verifyPNRStatus(void);
}

namespace {

std::vector<int> choices;
std::size_t choiceIndex = 0;

int registrationResult = 101;
int loginResult = 202;
int adminLoginResult = SUCCESS;

int initializeSystemCalls = 0;
int initializeTrainCalls = 0;
int initializeScheduleCalls = 0;
int initializeAvailabilityCalls = 0;
int initializeBookingCalls = 0;
int initializePassengerCalls = 0;
int initializeUserCalls = 0;
int cleanupCalls = 0;

int registerCalls = 0;
int loginCalls = 0;
int adminLoginCalls = 0;
int userMenuCalls = 0;
int adminMenuCalls = 0;
int pnrCalls = 0;

int receivedPassengerId = -1;

void resetState()
{
    choices.clear();
    choiceIndex = 0;

    registrationResult = 101;
    loginResult = 202;
    adminLoginResult = SUCCESS;

    initializeSystemCalls = 0;
    initializeTrainCalls = 0;
    initializeScheduleCalls = 0;
    initializeAvailabilityCalls = 0;
    initializeBookingCalls = 0;
    initializePassengerCalls = 0;
    initializeUserCalls = 0;
    cleanupCalls = 0;

    registerCalls = 0;
    loginCalls = 0;
    adminLoginCalls = 0;
    userMenuCalls = 0;
    adminMenuCalls = 0;
    pnrCalls = 0;

    receivedPassengerId = -1;
}

}  // namespace

// Test doubles for functions used by main.c.

extern "C" {

void initializeSystem(void)
{
    ++initializeSystemCalls;
}

void initializeTrainModule(void)
{
    ++initializeTrainCalls;
}

void initializeTrainScheduleModule(void)
{
    ++initializeScheduleCalls;
}

void initializeAvailabilityModule(void)
{
    ++initializeAvailabilityCalls;
}

void initializeBookingModule(void)
{
    ++initializeBookingCalls;
}

void initializePassengerModule(void)
{
    ++initializePassengerCalls;
}

void initializeUserModule(void)
{
    ++initializeUserCalls;
}

void cleanupSystem(void)
{
    ++cleanupCalls;
}

int getIntegerInput(const char* message)
{
    (void)message;

    if (choiceIndex >= choices.size()) {
        return 0;
    }

    return choices[choiceIndex++];
}

int registerUser(void)
{
    ++registerCalls;
    return registrationResult;
}

int loginUser(void)
{
    ++loginCalls;
    return loginResult;
}

int adminLogin(void)
{
    ++adminLoginCalls;
    return adminLoginResult;
}

void userMenu(int passengerId)
{
    ++userMenuCalls;
    receivedPassengerId = passengerId;
}

void adminMenu(void)
{
    ++adminMenuCalls;
}

void verifyPNRStatus(void)
{
    ++pnrCalls;
}

}  // extern "C"

class MainTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        resetState();
    }

    void expectAllModulesInitialized()
    {
        EXPECT_EQ(initializeSystemCalls, 1);
        EXPECT_EQ(initializeTrainCalls, 1);
        EXPECT_EQ(initializeScheduleCalls, 1);
        EXPECT_EQ(initializeAvailabilityCalls, 1);
        EXPECT_EQ(initializeBookingCalls, 1);
        EXPECT_EQ(initializePassengerCalls, 1);
        EXPECT_EQ(initializeUserCalls, 1);
    }
};

TEST_F(MainTest, ExitInitializesModulesAndCleansUp)
{
    choices = {0};

    EXPECT_EQ(railway_app_main(), 0);

    expectAllModulesInitialized();
    EXPECT_EQ(cleanupCalls, 1);
}

TEST_F(MainTest, SuccessfulRegistrationOpensUserMenu)
{
    registrationResult = 101;
    choices = {1, 0};

    EXPECT_EQ(railway_app_main(), 0);

    EXPECT_EQ(registerCalls, 1);
    EXPECT_EQ(userMenuCalls, 1);
    EXPECT_EQ(receivedPassengerId, 101);
    EXPECT_EQ(cleanupCalls, 1);
}

TEST_F(MainTest, FailedRegistrationDoesNotOpenUserMenu)
{
    registrationResult = FAILURE;
    choices = {1, 0};

    EXPECT_EQ(railway_app_main(), 0);

    EXPECT_EQ(registerCalls, 1);
    EXPECT_EQ(userMenuCalls, 0);
    EXPECT_EQ(cleanupCalls, 1);
}

TEST_F(MainTest, SuccessfulLoginOpensUserMenu)
{
    loginResult = 202;
    choices = {2, 0};

    EXPECT_EQ(railway_app_main(), 0);

    EXPECT_EQ(loginCalls, 1);
    EXPECT_EQ(userMenuCalls, 1);
    EXPECT_EQ(receivedPassengerId, 202);
}

TEST_F(MainTest, FailedLoginDoesNotOpenUserMenu)
{
    loginResult = FAILURE;
    choices = {2, 0};

    EXPECT_EQ(railway_app_main(), 0);

    EXPECT_EQ(loginCalls, 1);
    EXPECT_EQ(userMenuCalls, 0);
}

TEST_F(MainTest, SuccessfulAdminLoginOpensAdminMenu)
{
    adminLoginResult = SUCCESS;
    choices = {3, 0};

    EXPECT_EQ(railway_app_main(), 0);

    EXPECT_EQ(adminLoginCalls, 1);
    EXPECT_EQ(adminMenuCalls, 1);
}

TEST_F(MainTest, FailedAdminLoginDoesNotOpenAdminMenu)
{
    adminLoginResult = FAILURE;
    choices = {3, 0};

    EXPECT_EQ(railway_app_main(), 0);

    EXPECT_EQ(adminLoginCalls, 1);
    EXPECT_EQ(adminMenuCalls, 0);
}

TEST_F(MainTest, PnrChoiceVerifiesPnrStatus)
{
    choices = {4, 0};

    EXPECT_EQ(railway_app_main(), 0);

    EXPECT_EQ(pnrCalls, 1);
    EXPECT_EQ(cleanupCalls, 1);
}

TEST_F(MainTest, InvalidChoiceReturnsToMainMenu)
{
    choices = {99, 0};

    EXPECT_EQ(railway_app_main(), 0);

    EXPECT_EQ(registerCalls, 0);
    EXPECT_EQ(loginCalls, 0);
    EXPECT_EQ(adminLoginCalls, 0);
    EXPECT_EQ(adminMenuCalls, 0);
    EXPECT_EQ(pnrCalls, 0);
    EXPECT_EQ(cleanupCalls, 1);
}

TEST_F(MainTest, MultipleChoicesAreHandledInSequence)
{
    registrationResult = 101;
    loginResult = 202;
    adminLoginResult = SUCCESS;

    choices = {1, 2, 3, 4, 0};

    EXPECT_EQ(railway_app_main(), 0);

    EXPECT_EQ(registerCalls, 1);
    EXPECT_EQ(loginCalls, 1);
    EXPECT_EQ(adminLoginCalls, 1);
    EXPECT_EQ(userMenuCalls, 2);
    EXPECT_EQ(adminMenuCalls, 1);
    EXPECT_EQ(pnrCalls, 1);
    EXPECT_EQ(cleanupCalls, 1);
}