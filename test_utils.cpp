extern "C" {
#include "utils.h"
}

#include <gtest/gtest.h>
#include <cstdio>
#include <cstring>
#include <climits>
#include <csetjmp>
#include <cerrno>
#include <string>
#include <unistd.h>
#include <stdio_ext.h>

// Redirect stdin to controlled test input and restore it afterward.
class StdinRedirect {
public:
    explicit StdinRedirect(const std::string& input) {
        inputFile = tmpfile();
        if (inputFile == nullptr) {
            ADD_FAILURE() << "Could not create temporary input file";
            return;
        }

        fwrite(input.data(), 1, input.size(), inputFile);
        fflush(inputFile);
        rewind(inputFile);

        savedStdin = dup(STDIN_FILENO);
        if (savedStdin == -1 || dup2(fileno(inputFile), STDIN_FILENO) == -1) {
            ADD_FAILURE() << "Could not redirect stdin";
            if (savedStdin != -1) close(savedStdin);
            savedStdin = -1;
            fclose(inputFile);
            inputFile = nullptr;
            return;
        }

        clearerr(stdin);
        __fpurge(stdin);
    }

    ~StdinRedirect() {
        if (savedStdin != -1) {
            dup2(savedStdin, STDIN_FILENO);
            close(savedStdin);
        }

        clearerr(stdin);
        __fpurge(stdin);

        if (inputFile != nullptr) {
            fclose(inputFile);
        }
    }

    StdinRedirect(const StdinRedirect&) = delete;
    StdinRedirect& operator=(const StdinRedirect&) = delete;

private:
    FILE* inputFile = nullptr;
    int savedStdin = -1;
};

// Existing journey-date tests.
TEST(JourneyDateTest, AcceptsValidDates) {
    EXPECT_EQ(1, isValidJourneyDate("25:10:2026"));
    EXPECT_EQ(1, isValidJourneyDate("29:02:2024"));
    EXPECT_EQ(1, isValidJourneyDate("31:12:9999"));
}

TEST(JourneyDateTest, RejectsInvalidDates) {
    EXPECT_EQ(0, isValidJourneyDate("31:04:2026"));
    EXPECT_EQ(0, isValidJourneyDate("29:02:2023"));
    EXPECT_EQ(0, isValidJourneyDate("00:12:2026"));
    EXPECT_EQ(0, isValidJourneyDate("15:13:2026"));
}

TEST(JourneyDateTest, RejectsMalformedDates) {
    EXPECT_EQ(0, isValidJourneyDate(nullptr));
    EXPECT_EQ(0, isValidJourneyDate(""));
    EXPECT_EQ(0, isValidJourneyDate("2026-10-25"));
    EXPECT_EQ(0, isValidJourneyDate("1A:10:2026"));
    EXPECT_EQ(0, isValidJourneyDate("25:10:1899"));
    EXPECT_EQ(0, isValidJourneyDate("01:01:10000"));
}

// Integer input tests.
TEST(IntegerInputTest, AcceptsValidInteger) {
    StdinRedirect input("1279\n");
    EXPECT_EQ(1279, getIntegerInput("Number: "));
}

TEST(IntegerInputTest, RetriesInvalidAndEmptyInput) {
    StdinRedirect input("\nabc\n42\n");
    EXPECT_EQ(42, getIntegerInput("Number: "));
}

TEST(IntegerInputTest, AcceptsSurroundingWhitespace) {
    StdinRedirect input("  123  \n");
    EXPECT_EQ(123, getIntegerInput("Number: "));
}

TEST(IntegerInputTest, ReturnsZeroAtEndOfInput) {
    StdinRedirect input("");
    EXPECT_EQ(0, getIntegerInput("Number: "));
}

TEST(IntegerInputTest, RecoversFromOverlongInput) {
    StdinRedirect input(std::string(140, '7') + "\n42\n");
    EXPECT_EQ(42, getIntegerInput("Number: "));
}

// String input tests.
TEST(StringInputTest, TrimsLeadingWhitespace) {
    StdinRedirect input("   Railway Station\n");
    char buffer[64] = {};
    getStringInput("Name: ", buffer, sizeof(buffer));
    EXPECT_STREQ("Railway Station", buffer);
}

TEST(StringInputTest, RetriesEmptyInput) {
    StdinRedirect input("\n   \nChennai\n");
    char buffer[64] = {};
    getStringInput("Name: ", buffer, sizeof(buffer));
    EXPECT_STREQ("Chennai", buffer);
}

TEST(StringInputTest, HandlesOverlongInput) {
    StdinRedirect input("abcdefgh\nOK\n");
    char buffer[5] = {};
    getStringInput("Name: ", buffer, sizeof(buffer));
    EXPECT_STREQ("OK", buffer);
}

TEST(StringInputTest, ClearsBufferAtEndOfInput) {
    StdinRedirect input("");
    char buffer[32] = "old value";
    getStringInput("Name: ", buffer, sizeof(buffer));
    EXPECT_STREQ("", buffer);
}

// Floating-point input tests.
TEST(FloatInputTest, AcceptsValidFloat) {
    StdinRedirect input("125.50\n");
    EXPECT_FLOAT_EQ(125.50f, getFloatInput("Amount: "));
}

TEST(FloatInputTest, RetriesInvalidValues) {
    StdinRedirect input("\nabc\n42.5\n");
    EXPECT_FLOAT_EQ(42.5f, getFloatInput("Amount: "));
}

TEST(FloatInputTest, AcceptsNegativeAndZeroValues) {
    {
        StdinRedirect input("-25.5\n");
        EXPECT_FLOAT_EQ(-25.5f, getFloatInput("Amount: "));
    }
    {
        StdinRedirect input("0\n");
        EXPECT_FLOAT_EQ(0.0f, getFloatInput("Amount: "));
    }
}

TEST(FloatInputTest, ReturnsNegativeOneAtEndOfInput) {
    StdinRedirect input("");
    EXPECT_FLOAT_EQ(-1.0f, getFloatInput("Amount: "));
}

// Input-buffer and system lifecycle tests.
TEST(InputBufferTest, ClearsCharactersUntilNewline) {
    StdinRedirect input("discard this\n42\n");
    clearInputBuffer();
    EXPECT_EQ(42, getIntegerInput("Number: "));
}
TEST(IntegerInputTest, AcceptsInputWithoutTrailingNewline) {
    StdinRedirect input("123");
    EXPECT_EQ(123, getIntegerInput("Number: "));
}
TEST(SystemLifecycleTest, InitializesAndCleansUp) {
    EXPECT_NO_THROW(initializeSystem());
    EXPECT_NO_THROW(cleanupSystem());
}


TEST(IntegerInputTest, AcceptsIntegerBoundaries)
{
    {
        StdinRedirect input("2147483647\n");
        EXPECT_EQ(INT_MAX, getIntegerInput("Number: "));
    }

    {
        StdinRedirect input("-2147483648\n");
        EXPECT_EQ(INT_MIN, getIntegerInput("Number: "));
    }
}

TEST(IntegerInputTest, RejectsIntegerOverflow)
{
    StdinRedirect input("999999999999999999999999\n42\n");

    EXPECT_EQ(42, getIntegerInput("Number: "));
}

TEST(IntegerInputTest, RejectsTrailingCharacters)
{
    StdinRedirect input("12abc\n42\n");

    EXPECT_EQ(42, getIntegerInput("Number: "));
}

TEST(IntegerInputTest, ReturnsZeroAfterThreeInvalidInputs)
{
    StdinRedirect input("abc\n\n12abc\n");

    EXPECT_EQ(0, getIntegerInput("Number: "));
}

TEST(StringInputTest, RejectsNullBuffer)
{
    EXPECT_NO_THROW(getStringInput("Name: ", nullptr, 10));
}

TEST(StringInputTest, RejectsNonPositiveBufferSize)
{
    char buffer[10] = "original";

    getStringInput("Name: ", buffer, 0);
    EXPECT_STREQ("original", buffer);

    getStringInput("Name: ", buffer, -1);
    EXPECT_STREQ("original", buffer);
}

TEST(StringInputTest, ReturnsEmptyAfterThreeInvalidInputs)
{
    StdinRedirect input("\n   \n\t\n");
    char buffer[32] = "initial";

    getStringInput("Name: ", buffer, sizeof(buffer));

    EXPECT_STREQ("", buffer);
}

TEST(StringInputTest, AcceptsInputWithCarriageReturn)
{
    StdinRedirect input("  Chennai\r\n");
    char buffer[32] = {};

    getStringInput("Name: ", buffer, sizeof(buffer));

    EXPECT_STREQ("Chennai", buffer);
}

TEST(FloatInputTest, RejectsNonFiniteValues)
{
    StdinRedirect input("nan\n42.5\n");

    EXPECT_FLOAT_EQ(42.5f, getFloatInput("Amount: "));
}

TEST(FloatInputTest, RejectsValuesOutsideAllowedRange)
{
    StdinRedirect input("1000000001\n42.5\n");

    EXPECT_FLOAT_EQ(42.5f, getFloatInput("Amount: "));
}

TEST(FloatInputTest, RejectsTrailingCharacters)
{
    StdinRedirect input("12.5abc\n8.5\n");

    EXPECT_FLOAT_EQ(8.5f, getFloatInput("Amount: "));
}

TEST(FloatInputTest, ReturnsNegativeOneAfterThreeInvalidInputs)
{
    StdinRedirect input("abc\nnan\n\n");

    EXPECT_FLOAT_EQ(-1.0f, getFloatInput("Amount: "));
}

TEST(JourneyDateTest, ValidatesLeapYearRules)
{
    EXPECT_EQ(1, isValidJourneyDate("29:02:2000"));
    EXPECT_EQ(0, isValidJourneyDate("29:02:1900"));
    EXPECT_EQ(1, isValidJourneyDate("29:02:2024"));
    EXPECT_EQ(0, isValidJourneyDate("29:02:2025"));
}

TEST(JourneyDateTest, RejectsInvalidDayAndMonthBoundaries)
{
    EXPECT_EQ(0, isValidJourneyDate("32:01:2026"));
    EXPECT_EQ(0, isValidJourneyDate("31:06:2026"));
    EXPECT_EQ(0, isValidJourneyDate("00:01:2026"));
    EXPECT_EQ(0, isValidJourneyDate("01:00:2026"));
}

TEST(JourneyDateTest, AcceptsEarliestSupportedYear)
{
    EXPECT_EQ(1, isValidJourneyDate("01:01:1900"));
}



TEST(UtilsNavigationTest, RepeatedPromptReturnsToActiveMenu)
{
    // Prepare stdin before establishing the jump target.
    StdinRedirect input("\n\n\n\n");
    jmp_buf env;

    railway_set_navigation_target(&env);

    if (setjmp(env) == 0) {
        char buffer[32] = {};
        getStringInput("Repeated test prompt: ", buffer, sizeof(buffer));
        railway_set_navigation_target(nullptr);
        FAIL() << "Expected navigation after repeated prompts";
    }

    railway_set_navigation_target(nullptr);
}



TEST(UtilsNavigationTest, RepeatedPromptTriggersNavigation)
{
    // Construct the redirect before setjmp so longjmp doesn't skip
    // destruction of the StdinRedirect object.
    StdinRedirect input("\n\n\n\n");
    jmp_buf env;

    railway_set_navigation_target(&env);

    if (setjmp(env) == 0) {
        char buffer[32] = {};
        getStringInput("Repeated navigation prompt: ",
                       buffer, sizeof(buffer));

        railway_set_navigation_target(nullptr);
        FAIL() << "Expected navigation after repeated prompts";
    }

    railway_set_navigation_target(nullptr);
}

TEST(FloatInputTest, RetriesAfterReadFailure)
{
    // An overlong line causes readLine() to fail without reaching EOF.
    StdinRedirect input(std::string(140, 'x') + "\n12.5\n");

    EXPECT_FLOAT_EQ(12.5f, getFloatInput("Amount: "));
}

TEST(FloatInputTest, RejectsOverflowAndUnderflow)
{
    StdinRedirect input("1e9999\n1e-9999\n42.5\n");

    EXPECT_FLOAT_EQ(42.5f, getFloatInput("Amount: "));
}




