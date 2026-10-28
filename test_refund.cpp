extern "C" {
#include "refund.h"
#include "utils.h"
}

#include <gtest/gtest.h>
#include <cstdio>
#include <string>
#include <unistd.h>
#include <stdio_ext.h>

// Redirect stdin to controlled test input and restore it afterward.
class RefundStdinRedirect {
public:
    explicit RefundStdinRedirect(const std::string& input) {
        file = tmpfile();

        if (file == nullptr) {
            ADD_FAILURE() << "Could not create temporary input file";
            return;
        }

        fwrite(input.data(), 1, input.size(), file);
        fflush(file);
        rewind(file);

        saved = dup(STDIN_FILENO);

        if (saved == -1 || dup2(fileno(file), STDIN_FILENO) == -1) {
            ADD_FAILURE() << "Could not redirect stdin";

            if (saved != -1) {
                close(saved);
            }

            saved = -1;
            fclose(file);
            file = nullptr;
            return;
        }

        clearerr(stdin);
        __fpurge(stdin);
    }

    ~RefundStdinRedirect() {
        if (saved != -1) {
            dup2(saved, STDIN_FILENO);
            close(saved);
        }

        clearerr(stdin);
        __fpurge(stdin);

        if (file != nullptr) {
            fclose(file);
        }
    }

    RefundStdinRedirect(const RefundStdinRedirect&) = delete;
    RefundStdinRedirect& operator=(const RefundStdinRedirect&) = delete;

private:
    FILE* file = nullptr;
    int saved = -1;
};

// --------------------------------------------------
// calculateRefundAmount() tests
// --------------------------------------------------

TEST(RefundTest, CalculatesSeventyFivePercent) {
    EXPECT_FLOAT_EQ(75.0f, calculateRefundAmount(100.0f));
    EXPECT_FLOAT_EQ(37.5f, calculateRefundAmount(50.0f));
    EXPECT_FLOAT_EQ(0.75f, calculateRefundAmount(1.0f));
}

TEST(RefundTest, ZeroFareReturnsZero) {
    EXPECT_FLOAT_EQ(0.0f, calculateRefundAmount(0.0f));
}

TEST(RefundTest, NegativeFareReturnsZero) {
    EXPECT_FLOAT_EQ(0.0f, calculateRefundAmount(-100.0f));
}

// --------------------------------------------------
// confirmRefundAmount() tests
// --------------------------------------------------

TEST(RefundTest, ConfirmsCorrectAmountOnFirstAttempt) {
    RefundStdinRedirect input("75\n");

    EXPECT_EQ(1, confirmRefundAmount(100.0f, 75.0f));
}

TEST(RefundTest, ConfirmsCorrectAmountAfterOneIncorrectAttempt) {
    RefundStdinRedirect input("50\n75\n");

    EXPECT_EQ(1, confirmRefundAmount(100.0f, 75.0f));
}

TEST(RefundTest, ConfirmsCorrectAmountOnThirdAttempt) {
    RefundStdinRedirect input("10\n20\n75\n");

    EXPECT_EQ(1, confirmRefundAmount(100.0f, 75.0f));
}

TEST(RefundTest, RejectsThreeIncorrectAmounts) {
    RefundStdinRedirect input("10\n20\n30\n");

    EXPECT_EQ(0, confirmRefundAmount(100.0f, 75.0f));
}

TEST(RefundTest, AcceptsAmountWithinTolerance) {
    RefundStdinRedirect input("75.004\n");

    EXPECT_EQ(1, confirmRefundAmount(100.0f, 75.0f));
}



TEST(RefundTest, RejectsAmountOutsideTolerance) {
    RefundStdinRedirect input("75.02\n75\n");

    EXPECT_EQ(1, confirmRefundAmount(100.0f, 75.0f));
}

TEST(RefundTest, RejectsConfirmationWhenInputEnds) {
    RefundStdinRedirect input("");

    EXPECT_EQ(0, confirmRefundAmount(100.0f, 75.0f));
}