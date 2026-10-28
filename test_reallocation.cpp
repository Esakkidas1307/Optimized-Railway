#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "reallocation.h"
#include "reserved_booking.h"
#include "availability.h"

// Isolated test definition; do not link another definition of bookingHead.
Booking *bookingHead = nullptr;
}

class ReallocationTest : public ::testing::Test {
protected:
    Booking *cancelled = nullptr;
    Booking *queued1 = nullptr;
    Booking *queued2 = nullptr;
    Availability inventory{};
    ReallocationResult result{};

    void SetUp() override {
        bookingHead = nullptr;
        inventory.availableSeats = 0;
        inventory.availableRAC = 0;
        inventory.availableWL = 0;
        std::memset(&result, 0, sizeof(result));
    }

    void TearDown() override {
        delete cancelled;
        delete queued1;
        delete queued2;
        cancelled = nullptr;
        queued1 = nullptr;
        queued2 = nullptr;
        bookingHead = nullptr;
    }

    Booking *makeBooking(
        const char *pnr,
        int status,
        const char *type = "RESERVED")
    {
        Booking *b = new Booking{};

        std::snprintf(b->pnrNumber, sizeof(b->pnrNumber), "%s", pnr);
        std::snprintf(b->bookingType, sizeof(b->bookingType), "%s", type);
        std::snprintf(b->journeyDate, sizeof(b->journeyDate), "%s", "2026-12-01");
        std::snprintf(b->className, sizeof(b->className), "%s", "SL");

        b->trainNumber = 12345;
        b->ticketStatus = status;
        b->next = nullptr;

        return b;
    }

    void addToList(Booking *b) {
        if (bookingHead == nullptr) {
            bookingHead = b;
            return;
        }

        Booking *tail = bookingHead;
        while (tail->next != nullptr) {
            tail = tail->next;
        }
        tail->next = b;
    }
};

TEST_F(ReallocationTest, NullBookingReturnsZero) {
    EXPECT_EQ(
        reallocateAfterCancellation(nullptr, &inventory, 10, 5, &result),
        0);
}

TEST_F(ReallocationTest, NullInventoryReturnsZero) {
    cancelled = makeBooking("P100", CNF);

    EXPECT_EQ(
        reallocateAfterCancellation(cancelled, nullptr, 10, 5, &result),
        0);
}

TEST_F(ReallocationTest, NullResultReturnsZero) {
    cancelled = makeBooking("P100", CNF);

    EXPECT_EQ(
        reallocateAfterCancellation(cancelled, &inventory, 10, 5, nullptr),
        0);
}

TEST_F(ReallocationTest, ConfirmedCancellationAtSeatLimitReturnsZero) {
    cancelled = makeBooking("P100", CNF);
    inventory.availableSeats = 10;

    EXPECT_EQ(
        reallocateAfterCancellation(cancelled, &inventory, 10, 5, &result),
        0);

    EXPECT_EQ(cancelled->ticketStatus, CNF);
    EXPECT_EQ(inventory.availableSeats, 10);
}

TEST_F(ReallocationTest, RacCancellationAtRacLimitReturnsZero) {
    cancelled = makeBooking("P100", RAC);
    inventory.availableRAC = 5;

    EXPECT_EQ(
        reallocateAfterCancellation(cancelled, &inventory, 10, 5, &result),
        0);

    EXPECT_EQ(cancelled->ticketStatus, RAC);
    EXPECT_EQ(inventory.availableRAC, 5);
}

TEST_F(ReallocationTest, UnsupportedStatusReturnsZero) {
    cancelled = makeBooking("P100", CANCELLED);

    EXPECT_EQ(
        reallocateAfterCancellation(cancelled, &inventory, 10, 5, &result),
        0);

    EXPECT_EQ(cancelled->ticketStatus, CANCELLED);
}

TEST_F(ReallocationTest, WaitlistedCancellationSucceeds) {
    cancelled = makeBooking("P100", WL);
    inventory.availableRAC = 0;

    EXPECT_EQ(
        reallocateAfterCancellation(cancelled, &inventory, 10, 5, &result),
        1);

    EXPECT_EQ(cancelled->ticketStatus, CANCELLED);
    EXPECT_EQ(inventory.availableSeats, 0);
    EXPECT_EQ(inventory.availableRAC, 0);
}

TEST_F(ReallocationTest, ConfirmedCancellationPromotesOldestRac) {
    cancelled = makeBooking("P100", CNF);
    queued1 = makeBooking("P300", RAC);
    queued2 = makeBooking("P200", RAC);

    addToList(cancelled);
    addToList(queued1);
    addToList(queued2);

    inventory.availableSeats = 0;
    inventory.availableRAC = 1;

    ASSERT_EQ(
        reallocateAfterCancellation(cancelled, &inventory, 10, 5, &result),
        1);

    EXPECT_EQ(cancelled->ticketStatus, CANCELLED);

    // The implementation selects the lexicographically smallest PNR.
    EXPECT_EQ(queued2->ticketStatus, CNF);
    EXPECT_EQ(queued1->ticketStatus, RAC);
    EXPECT_EQ(result.promotedToCNF, queued2);
    EXPECT_EQ(result.promotedToCNFOldStatus, RAC);
    EXPECT_EQ(inventory.availableSeats, 0);
}

TEST_F(ReallocationTest, ConfirmedCancellationPromotesWaitlistToConfirmed) {
    cancelled = makeBooking("P100", CNF);
    queued1 = makeBooking("P200", WL);

    addToList(cancelled);
    addToList(queued1);

    inventory.availableSeats = 0;
    inventory.availableRAC = 0;

    ASSERT_EQ(
        reallocateAfterCancellation(cancelled, &inventory, 10, 5, &result),
        1);

    EXPECT_EQ(cancelled->ticketStatus, CANCELLED);
    EXPECT_EQ(queued1->ticketStatus, CNF);
    EXPECT_EQ(result.promotedToCNF, queued1);
    EXPECT_EQ(result.promotedToCNFOldStatus, WL);
    EXPECT_EQ(inventory.availableSeats, 0);
}

TEST_F(ReallocationTest, AvailableRacPromotesWaitlistedBookingToRac) {
    cancelled = makeBooking("P100", WL);
    queued1 = makeBooking("P200", WL);

    addToList(cancelled);
    addToList(queued1);

    inventory.availableRAC = 1;

    ASSERT_EQ(
        reallocateAfterCancellation(cancelled, &inventory, 10, 5, &result),
        1);

    EXPECT_EQ(cancelled->ticketStatus, CANCELLED);
    EXPECT_EQ(queued1->ticketStatus, RAC);
    EXPECT_EQ(result.promotedToRAC, queued1);
    EXPECT_EQ(inventory.availableRAC, 0);
}

TEST_F(ReallocationTest, UnreservedBookingIsNotPromoted) {
    cancelled = makeBooking("P100", CNF);
    queued1 = makeBooking("P200", RAC, "UNRESERVED");

    addToList(cancelled);
    addToList(queued1);

    inventory.availableSeats = 0;
    inventory.availableRAC = 0;

    ASSERT_EQ(
        reallocateAfterCancellation(cancelled, &inventory, 10, 5, &result),
        1);

    EXPECT_EQ(queued1->ticketStatus, RAC);
    EXPECT_EQ(result.promotedToCNF, nullptr);
}

TEST_F(ReallocationTest, RollbackRestoresBookingAndInventory) {
    cancelled = makeBooking("P100", CNF);
    queued1 = makeBooking("P200", WL);

    addToList(cancelled);
    addToList(queued1);

    inventory.availableSeats = 2;
    inventory.availableRAC = 0;
    inventory.availableWL = 3;

    ASSERT_EQ(
        reallocateAfterCancellation(cancelled, &inventory, 10, 5, &result),
        1);

    EXPECT_EQ(cancelled->ticketStatus, CANCELLED);
    EXPECT_EQ(queued1->ticketStatus, CNF);

    rollbackReallocation(cancelled, &inventory, &result);

    EXPECT_EQ(cancelled->ticketStatus, CNF);
    EXPECT_EQ(queued1->ticketStatus, WL);
    EXPECT_EQ(inventory.availableSeats, 2);
    EXPECT_EQ(inventory.availableRAC, 0);
    EXPECT_EQ(inventory.availableWL, 3);
}

TEST_F(ReallocationTest, RollbackWithNullArgumentsDoesNotCrash) {
    cancelled = makeBooking("P100", CNF);

    EXPECT_NO_THROW(rollbackReallocation(nullptr, &inventory, &result));
    EXPECT_NO_THROW(rollbackReallocation(cancelled, nullptr, &result));
    EXPECT_NO_THROW(rollbackReallocation(cancelled, &inventory, nullptr));
}

TEST_F(ReallocationTest, RacCancellationAtCapacityReturnsZero)
{
    cancelled = makeBooking("PNR100", RAC);
    inventory.availableRAC = 2;

    EXPECT_EQ(reallocateAfterCancellation(
                  cancelled, &inventory, 10, 2, &result),
              0);

    EXPECT_EQ(cancelled->ticketStatus, RAC);
    EXPECT_EQ(inventory.availableRAC, 2);
}

TEST_F(ReallocationTest, ConfirmedCancellationPromotesWaitlistWhenNoRacExists)
{
    cancelled = makeBooking("PNR100", CNF);
    queued1 = makeBooking("PNR200", WL);

    addToList(cancelled);
    addToList(queued1);

    inventory.availableSeats = 0;
    inventory.availableRAC = 0;

    ASSERT_EQ(reallocateAfterCancellation(
                  cancelled, &inventory, 10, 5, &result),
              1);

    EXPECT_EQ(cancelled->ticketStatus, CANCELLED);
    EXPECT_EQ(queued1->ticketStatus, CNF);
    EXPECT_EQ(result.promotedToCNF, queued1);
    EXPECT_EQ(result.promotedToCNFOldStatus, WL);
}

TEST_F(ReallocationTest, DoesNotPromoteBookingFromDifferentTrain)
{
    cancelled = makeBooking("PNR100", CNF);
    queued1 = makeBooking("PNR200", WL);

    queued1->trainNumber = 99999;

    addToList(cancelled);
    addToList(queued1);

    inventory.availableSeats = 0;
    inventory.availableRAC = 0;

    ASSERT_EQ(reallocateAfterCancellation(
                  cancelled, &inventory, 10, 5, &result),
              1);

    EXPECT_EQ(queued1->ticketStatus, WL);
    EXPECT_EQ(result.promotedToCNF, nullptr);
}

TEST_F(ReallocationTest, DoesNotPromoteBookingFromDifferentDate)
{
    cancelled = makeBooking("PNR100", CNF);
    queued1 = makeBooking("PNR200", WL);

    std::strcpy(queued1->journeyDate, "2026-12-02");

    addToList(cancelled);
    addToList(queued1);

    inventory.availableSeats = 0;
    inventory.availableRAC = 0;

    ASSERT_EQ(reallocateAfterCancellation(
                  cancelled, &inventory, 10, 5, &result),
              1);

    EXPECT_EQ(queued1->ticketStatus, WL);
    EXPECT_EQ(result.promotedToCNF, nullptr);
}

TEST_F(ReallocationTest, DoesNotPromoteBookingFromDifferentClass)
{
    cancelled = makeBooking("PNR100", CNF);
    queued1 = makeBooking("PNR200", WL);

    std::strcpy(queued1->className, "3A");

    addToList(cancelled);
    addToList(queued1);

    inventory.availableSeats = 0;
    inventory.availableRAC = 0;

    ASSERT_EQ(reallocateAfterCancellation(
                  cancelled, &inventory, 10, 5, &result),
              1);

    EXPECT_EQ(queued1->ticketStatus, WL);
    EXPECT_EQ(result.promotedToCNF, nullptr);
}

TEST_F(ReallocationTest, RollbackRestoresBothPromotedBookings)
{
    cancelled = makeBooking("PNR100", CNF);
    queued1 = makeBooking("PNR200", RAC);
    queued2 = makeBooking("PNR300", WL);

    addToList(cancelled);
    addToList(queued1);
    addToList(queued2);

    inventory.availableSeats = 0;
    inventory.availableRAC = 1;

    ASSERT_EQ(reallocateAfterCancellation(
                  cancelled, &inventory, 10, 5, &result),
              1);

    ASSERT_NE(result.promotedToCNF, nullptr);
    ASSERT_NE(result.promotedToRAC, nullptr);

    rollbackReallocation(cancelled, &inventory, &result);

    EXPECT_EQ(cancelled->ticketStatus, CNF);
    EXPECT_EQ(queued1->ticketStatus, RAC);
    EXPECT_EQ(queued2->ticketStatus, WL);
}

