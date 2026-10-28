#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <cstdio>

extern "C" {
#include "pnr_status.h"
#include "reserved_booking.h"
#include "utils.h"

Booking *bookingHead = nullptr;

// Match the declarations in the project headers.
void getStringInput(const char *message, char *buffer, int size);
void printBookingTicket(const Booking *booking);
}

static std::string inputPNR;
static int ticketPrintCount = 0;
static const Booking *printedBooking = nullptr;

// Stub user input so tests do not wait for interactive input.
extern "C" void getStringInput(const char *message, char *buffer, int size)
{
    (void)message;

    if (buffer == nullptr || size <= 0)
        return;

    std::snprintf(buffer, static_cast<size_t>(size), "%s", inputPNR.c_str());
}

// Stub ticket printing and record which booking was printed.
extern "C" void printBookingTicket(const Booking *booking)
{
    ticketPrintCount++;
    printedBooking = booking;
}

class PNRStatusTest : public ::testing::Test {
protected:
    Booking *originalHead = nullptr;
    Booking *firstBooking = nullptr;
    Booking *secondBooking = nullptr;

    void SetUp() override
    {
        originalHead = bookingHead;
        bookingHead = nullptr;

        inputPNR.clear();
        ticketPrintCount = 0;
        printedBooking = nullptr;
    }

    void TearDown() override
    {
        // Restore the original global list.
        bookingHead = originalHead;

        // Free only the bookings created by this test.
        delete firstBooking;
        delete secondBooking;

        firstBooking = nullptr;
        secondBooking = nullptr;
    }

    Booking *makeBooking(
        const char *type,
        const char *pnr,
        Booking *next = nullptr)
    {
        Booking *booking = new Booking{};

        std::snprintf(
            booking->bookingType,
            sizeof(booking->bookingType),
            "%s",
            type);

        std::snprintf(
            booking->pnrNumber,
            sizeof(booking->pnrNumber),
            "%s",
            pnr);

        booking->next = next;
        return booking;
    }
};

TEST_F(PNRStatusTest, ValidReservedPNRPrintsTicket)
{
    firstBooking = makeBooking("RESERVED", "PNR123");
    bookingHead = firstBooking;
    inputPNR = "PNR123";

    testing::internal::CaptureStdout();
    verifyPNRStatus();
    const std::string output =
        testing::internal::GetCapturedStdout();

    EXPECT_NE(
        output.find("PNR verified successfully"),
        std::string::npos);
    EXPECT_EQ(ticketPrintCount, 1);
    EXPECT_EQ(printedBooking, firstBooking);
}

TEST_F(PNRStatusTest, UnreservedBookingIsRejected)
{
    firstBooking = makeBooking("UNRESERVED", "PNR123");
    bookingHead = firstBooking;
    inputPNR = "PNR123";

    testing::internal::CaptureStdout();
    verifyPNRStatus();
    const std::string output =
        testing::internal::GetCapturedStdout();

    EXPECT_NE(
        output.find("Invalid PNR"),
        std::string::npos);
    EXPECT_EQ(ticketPrintCount, 0);
}

TEST_F(PNRStatusTest, EmptyStoredPNRIsRejected)
{
    firstBooking = makeBooking("RESERVED", "");
    bookingHead = firstBooking;
    inputPNR = "";

    testing::internal::CaptureStdout();
    verifyPNRStatus();
    const std::string output =
        testing::internal::GetCapturedStdout();

    EXPECT_NE(
        output.find("Invalid PNR"),
        std::string::npos);
    EXPECT_EQ(ticketPrintCount, 0);
}

TEST_F(PNRStatusTest, IncorrectPNRIsRejected)
{
    firstBooking = makeBooking("RESERVED", "PNR123");
    bookingHead = firstBooking;
    inputPNR = "WRONGPNR";

    testing::internal::CaptureStdout();
    verifyPNRStatus();
    const std::string output =
        testing::internal::GetCapturedStdout();

    EXPECT_NE(
        output.find("Invalid PNR"),
        std::string::npos);
    EXPECT_EQ(ticketPrintCount, 0);
}

TEST_F(PNRStatusTest, EmptyBookingListRejectsPNR)
{
    bookingHead = nullptr;
    inputPNR = "PNR123";

    testing::internal::CaptureStdout();
    verifyPNRStatus();
    const std::string output =
        testing::internal::GetCapturedStdout();

    EXPECT_NE(
        output.find("Invalid PNR"),
        std::string::npos);
    EXPECT_EQ(ticketPrintCount, 0);
}

TEST_F(PNRStatusTest, FindsMatchingPNRAfterNonMatchingBooking)
{
    firstBooking = makeBooking("RESERVED", "PNR123");
    secondBooking = makeBooking("RESERVED", "PNR456");

    firstBooking->next = secondBooking;
    bookingHead = firstBooking;
    inputPNR = "PNR456";

    testing::internal::CaptureStdout();
    verifyPNRStatus();
    const std::string output =
        testing::internal::GetCapturedStdout();

    EXPECT_NE(
        output.find("PNR verified successfully"),
        std::string::npos);
    EXPECT_EQ(ticketPrintCount, 1);
    EXPECT_EQ(printedBooking, secondBooking);
}
