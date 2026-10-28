#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <algorithm>
#include <iterator>
#include <vector>

/*
 * Use C linkage for declarations implemented in the C source files.
 */
extern "C" {
#include "reserved_booking.h"
#include "reallocation.h"
#include "refund.h"
#include "availability.h"
#include "train.h"
#include "utils.h"
}

/* ---------------------------------------------------------
 * Test-controlled state
 * --------------------------------------------------------- */

extern "C" {
Booking *bookingHead = nullptr;
}

namespace {

Booking booking;
Booking queuedBooking;
Availability inventory;
Train train;

std::vector<std::string> stringInputs;
size_t nextStringInput = 0;

bool availabilityFound = true;
bool trainFound = true;
bool refundConfirmed = true;
bool reallocationSucceeds = true;

int bookingSaveResult = SUCCESS;
int availabilitySaveResult = SUCCESS;

int bookingSaveCalls = 0;
int availabilitySaveCalls = 0;
int rollbackCalls = 0;
int reallocationCalls = 0;

Booking *promoteToCNF = nullptr;
Booking *promoteToRAC = nullptr;

void copyText(char *destination, size_t size, const char *source)
{
    std::snprintf(destination, size, "%s", source);
}

void resetTestState()
{

    booking = {};
    queuedBooking = {};
    inventory = {};
    train = {};

    bookingHead = &booking;

    copyText(booking.pnrNumber, sizeof booking.pnrNumber, "PNR001");
    booking.passengerId = 10;
    copyText(booking.passengerName, sizeof booking.passengerName, "Test Passenger");
    booking.trainNumber = 12345;
    copyText(booking.trainName, sizeof booking.trainName, "Test Express");
    copyText(booking.journeyDate, sizeof booking.journeyDate, "2026-11-01");
    copyText(booking.className, sizeof booking.className, "SL");
    copyText(booking.bookingType, sizeof booking.bookingType, "RESERVED");
    booking.fare = 1000.0f;
    booking.ticketStatus = CNF;

    booking.prev = nullptr;
    booking.next = nullptr;

    inventory.trainNumber = 12345;
    copyText(inventory.journeyDate, sizeof inventory.journeyDate, "2026-11-01");
    copyText(inventory.className, sizeof inventory.className, "SL");
    inventory.availableSeats = 5;
    inventory.availableRAC = 2;
    inventory.availableWL = 3;

    train.trainNumber = 12345;
    train.classCount = 1;
    copyText(train.coaches[0].className,
             sizeof train.coaches[0].className, "SL");
    train.coaches[0].totalSeats = 100;
    train.coaches[0].racCapacity = 10;

    stringInputs = {"PNR001", "Y"};
    nextStringInput = 0;

    availabilityFound = true;
    trainFound = true;
    refundConfirmed = true;
    reallocationSucceeds = true;

    bookingSaveResult = SUCCESS;
    availabilitySaveResult = SUCCESS;

    bookingSaveCalls = 0;
    availabilitySaveCalls = 0;
    rollbackCalls = 0;
    reallocationCalls = 0;

    promoteToCNF = nullptr;
    promoteToRAC = nullptr;
}

void setInputs(std::initializer_list<const char *> inputs)
{
    stringInputs.clear();

    std::transform(
    inputs.begin(),
    inputs.end(),
    std::back_inserter(stringInputs),
    [](const char *input) {
        return std::string(input);
    }
);
    nextStringInput = 0;
}

} // namespace

/* ---------------------------------------------------------
 * C dependency test doubles
 * --------------------------------------------------------- */

extern "C" {

void getStringInput(const char *, char *buffer, int size)
{
    std::string input = "";

    if (nextStringInput < stringInputs.size())
        input = stringInputs[nextStringInput++];

    if (size <= 0)
        return;

    std::snprintf(buffer, static_cast<size_t>(size), "%s", input.c_str());
}

float calculateRefundAmount(float farePaid)
{
    if (farePaid < 0.0f)
        return 0.0f;

    return farePaid * 0.75f;
}

int confirmRefundAmount(float, float)
{
    return refundConfirmed ? 1 : 0;
}

Availability *findAvailability(int trainNumber,
                               const char *journeyDate,
                               const char *className)
{
    if (!availabilityFound)
        return nullptr;

    if (inventory.trainNumber != trainNumber)
        return nullptr;

    if (std::strcmp(inventory.journeyDate, journeyDate) != 0)
        return nullptr;

    if (std::strcmp(inventory.className, className) != 0)
        return nullptr;

    return &inventory;
}

Train *findTrain(int trainNumber)
{
    if (!trainFound || train.trainNumber != trainNumber)
        return nullptr;

    return &train;
}

int saveBookingData(void)
{
    ++bookingSaveCalls;
    return bookingSaveResult;
}

int saveAvailabilityData(void)
{
    ++availabilitySaveCalls;
    return availabilitySaveResult;
}

int reallocateAfterCancellation(Booking *booking,
                                Availability *const inventory,
                                int maxSeats,
                                int maxRac,
                                ReallocationResult *r)
{
    (void)maxSeats;
    (void)maxRac;

    ++reallocationCalls;

    if (!reallocationSucceeds)
    {
        return 0;
    }

    if ((booking == nullptr) ||
        (inventory == nullptr) ||
        (r == nullptr))
    {
        return 0;
    }

    r->oldTicketStatus = booking->ticketStatus;
    r->oldAvailableSeats = inventory->availableSeats;
    r->oldAvailableRAC = inventory->availableRAC;
    r->oldAvailableWL = inventory->availableWL;

    r->promotedToCNF = promoteToCNF;
    r->promotedToRAC = promoteToRAC;

    r->promotedToCNFOldStatus =
        (promoteToCNF != nullptr)
            ? promoteToCNF->ticketStatus
            : 0;

    booking->ticketStatus = CANCELLED;

    if (promoteToCNF != nullptr)
    {
        promoteToCNF->ticketStatus = CNF;
    }

    if (promoteToRAC != nullptr)
    {
        promoteToRAC->ticketStatus = RAC;
    }

    return 1;
}


void rollbackReallocation(Booking *booking,
                          Availability *inventory,
                          const ReallocationResult *r)
{
    if ((booking == nullptr) ||
        (inventory == nullptr) ||
        (r == nullptr))
    {
        return;
    }

    ++rollbackCalls;

    booking->ticketStatus = r->oldTicketStatus;

    if (r->promotedToCNF != nullptr)
    {
        r->promotedToCNF->ticketStatus =
            r->promotedToCNFOldStatus;
    }

    if (r->promotedToRAC != nullptr)
    {
        r->promotedToRAC->ticketStatus = WL;
    }

    inventory->availableSeats = r->oldAvailableSeats;
    inventory->availableRAC = r->oldAvailableRAC;
    inventory->availableWL = r->oldAvailableWL;
} 
}// extern"C"
/* ---------------------------------------------------------
 * Google Test fixture
 * --------------------------------------------------------- */

class CancellationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        resetTestState();
    }

    void cancel()
    {
        cancelTicket(10);
    }
};

/* ---------------------------------------------------------
 * Early-return paths
 * --------------------------------------------------------- */

TEST_F(CancellationTest, BookingNotFound)
{
    setInputs({"UNKNOWN"});

    cancel();

    EXPECT_EQ(booking.ticketStatus, CNF);
    EXPECT_EQ(reallocationCalls, 0);
    EXPECT_EQ(bookingSaveCalls, 0);
}

TEST_F(CancellationTest, BookingBelongsToAnotherPassenger)
{
    booking.passengerId = 20;
    setInputs({"PNR001"});

    cancel();

    EXPECT_EQ(booking.ticketStatus, CNF);
    EXPECT_EQ(reallocationCalls, 0);
}

TEST_F(CancellationTest, UnreservedTicketIsRejected)
{
    copyText(booking.bookingType, sizeof booking.bookingType, "UNRESERVED");

    cancel();

    EXPECT_EQ(booking.ticketStatus, CNF);
    EXPECT_EQ(reallocationCalls, 0);
}

TEST_F(CancellationTest, AlreadyCancelledTicketIsRejected)
{
    booking.ticketStatus = CANCELLED;

    cancel();

    EXPECT_EQ(booking.ticketStatus, CANCELLED);
    EXPECT_EQ(reallocationCalls, 0);
}

/* ---------------------------------------------------------
 * Confirmation paths
 * --------------------------------------------------------- */

TEST_F(CancellationTest, InvalidYesNoAnswerIsRetried)
{
    setInputs({"PNR001", "maybe", "Y"});

    cancel();

    EXPECT_EQ(booking.ticketStatus, CANCELLED);
    EXPECT_EQ(reallocationCalls, 1);
}

TEST_F(CancellationTest, UserDeclinesCancellation)
{
    setInputs({"PNR001", "N"});

    cancel();

    EXPECT_EQ(booking.ticketStatus, CNF);
    EXPECT_EQ(reallocationCalls, 0);
    EXPECT_EQ(bookingSaveCalls, 0);
}

TEST_F(CancellationTest, RefundConfirmationRejected)
{
    refundConfirmed = false;

    cancel();

    EXPECT_EQ(booking.ticketStatus, CNF);
    EXPECT_EQ(reallocationCalls, 0);
    EXPECT_EQ(bookingSaveCalls, 0);
}

/* ---------------------------------------------------------
 * Inventory and capacity paths
 * --------------------------------------------------------- */

TEST_F(CancellationTest, MissingAvailabilityStopsCancellation)
{
    availabilityFound = false;

    cancel();

    EXPECT_EQ(booking.ticketStatus, CNF);
    EXPECT_EQ(reallocationCalls, 0);
}

TEST_F(CancellationTest, MissingTrainStopsCancellation)
{
    trainFound = false;

    cancel();

    EXPECT_EQ(booking.ticketStatus, CNF);
    EXPECT_EQ(reallocationCalls, 0);
}

TEST_F(CancellationTest, MissingClassCapacityStopsCancellation)
{
    train.classCount = 0;

    cancel();

    EXPECT_EQ(booking.ticketStatus, CNF);
    EXPECT_EQ(reallocationCalls, 0);
}

TEST_F(CancellationTest, NonMatchingClassCapacityStopsCancellation)
{
    copyText(train.coaches[0].className,
             sizeof train.coaches[0].className, "AC");

    cancel();

    EXPECT_EQ(booking.ticketStatus, CNF);
    EXPECT_EQ(reallocationCalls, 0);
}

/* ---------------------------------------------------------
 * Reallocation and save failures
 * --------------------------------------------------------- */

TEST_F(CancellationTest, ReallocationFailureStopsCancellation)
{
    reallocationSucceeds = false;

    cancel();

    EXPECT_EQ(booking.ticketStatus, CNF);
    EXPECT_EQ(bookingSaveCalls, 0);
}

TEST_F(CancellationTest, BookingSaveFailureTriggersRollback)
{
    bookingSaveResult = FAILURE;

    cancel();

    EXPECT_EQ(booking.ticketStatus, CNF);
    EXPECT_EQ(rollbackCalls, 1);
    EXPECT_EQ(bookingSaveCalls, 2);
    EXPECT_EQ(availabilitySaveCalls, 1);
}

TEST_F(CancellationTest, AvailabilitySaveFailureTriggersRollback)
{
    availabilitySaveResult = FAILURE;

    cancel();

    EXPECT_EQ(booking.ticketStatus, CNF);
    EXPECT_EQ(rollbackCalls, 1);
    EXPECT_EQ(bookingSaveCalls, 2);
    EXPECT_EQ(availabilitySaveCalls, 2);
}

/* ---------------------------------------------------------
 * Successful cancellation paths
 * --------------------------------------------------------- */

TEST_F(CancellationTest, SuccessfulConfirmedTicketCancellation)
{
    cancel();

    EXPECT_EQ(booking.ticketStatus, CANCELLED);
    EXPECT_EQ(bookingSaveCalls, 1);
    EXPECT_EQ(availabilitySaveCalls, 1);
    EXPECT_EQ(rollbackCalls, 0);
}

TEST_F(CancellationTest, SuccessfulRacTicketCancellation)
{
    booking.ticketStatus = RAC;

    cancel();

    EXPECT_EQ(booking.ticketStatus, CANCELLED);
    EXPECT_EQ(bookingSaveCalls, 1);
}

TEST_F(CancellationTest, SuccessfulWaitingListCancellation)
{
    booking.ticketStatus = WL;

    cancel();

    EXPECT_EQ(booking.ticketStatus, CANCELLED);
    EXPECT_EQ(bookingSaveCalls, 1);
}

TEST_F(CancellationTest, UnknownTicketStatusReachesReallocationFailure)
{
    booking.ticketStatus = 99;
    reallocationSucceeds = false;

    cancel();

    EXPECT_EQ(booking.ticketStatus, 99);
    EXPECT_EQ(reallocationCalls, 1);
}

/* ---------------------------------------------------------
 * Promotion messages
 * --------------------------------------------------------- */

TEST_F(CancellationTest, SuccessfulCancellationWithCNFPromotion)
{
    queuedBooking.ticketStatus = RAC;
    copyText(queuedBooking.pnrNumber,
             sizeof queuedBooking.pnrNumber, "PNR002");

    promoteToCNF = &queuedBooking;

    cancel();

    EXPECT_EQ(booking.ticketStatus, CANCELLED);
    EXPECT_EQ(queuedBooking.ticketStatus, CNF);
}

TEST_F(CancellationTest, SuccessfulCancellationWithRACPromotion)
{
    queuedBooking.ticketStatus = WL;
    copyText(queuedBooking.pnrNumber,
             sizeof queuedBooking.pnrNumber, "PNR003");

    promoteToRAC = &queuedBooking;

    cancel();

    EXPECT_EQ(booking.ticketStatus, CANCELLED);
    EXPECT_EQ(queuedBooking.ticketStatus, RAC);
}

TEST_F(CancellationTest, SuccessfulCancellationWithoutPromotion)
{
    promoteToCNF = nullptr;
    promoteToRAC = nullptr;

    cancel();

    EXPECT_EQ(booking.ticketStatus, CANCELLED);
    EXPECT_EQ(bookingSaveCalls, 1);
}
