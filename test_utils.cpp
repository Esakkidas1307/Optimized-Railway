extern "C" {
#include "utils.h"
}

#include <gtest/gtest.h>
#include <cstdio>
#include <cstring>
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
