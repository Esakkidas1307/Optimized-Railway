#include "reserved_booking.h"
#include "train.h"
#include "utils.h"
#include "availability.h"
#include "refund.h"
#include "reallocation.h"

static int askYesNo(const char *prompt)
{
    char answer[16];
    for (;;) {
        getStringInput(prompt, answer, sizeof answer);
        if (answer[0] == 'Y' || answer[0] == 'y') return 1;
        if (answer[0] == 'N' || answer[0] == 'n') return 0;
        printf("Please enter Y or N.\n");
    }
}

void cancelTicket(int passengerId)
{
    char pnr[20];
    getStringInput("Enter reserved-ticket PNR: ", pnr, sizeof pnr);
    Booking *booking = bookingHead;
    while (booking && !(strcmp(booking->pnrNumber, pnr) == 0 && booking->passengerId == passengerId))
        booking = booking->next;
    if (!booking) { printf("\nBooking not found for this user. Check the PNR.\n"); return; }
    if (strcmp(booking->bookingType, "UNRESERVED") == 0) { printf("\nUnreserved-ticket cancellation is not supported by this cancellation workflow yet.\n"); return; }
    if (booking->ticketStatus == CANCELLED) { printf("\nThis ticket has already been cancelled.\n"); return; }

    printf("\nTicket found: PNR %s | Train %d - %s | Date %s | Class %s | Fare paid Rs. %.2f | Status %s\n",
        booking->pnrNumber, booking->trainNumber, booking->trainName, booking->journeyDate,
        booking->className, booking->fare, booking->ticketStatus == CNF ? "CNF" :
        booking->ticketStatus == RAC ? "RAC" : booking->ticketStatus == WL ? "WL" : "UNKNOWN");
    if (!askYesNo("Do you want to cancel this ticket? (Y/N) : ")) {
        printf("Cancellation aborted. Your ticket remains unchanged.\n"); return;
    }

    float refund = calculateRefundAmount(booking->fare);
    if (!confirmRefundAmount(booking->fare, refund)) return;

    Availability *inventory = findAvailability(booking->trainNumber, booking->journeyDate, booking->className);
    if (!inventory) { printf("Cancellation stopped: matching inventory record was not found.\n"); return; }
    Train *train = findTrain(booking->trainNumber);
    int maxSeats = -1, maxRac = -1;
    if (train) for (int i = 0; i < train->classCount; ++i) {
        if (strcmp(train->coaches[i].className, booking->className) == 0) {
            maxSeats = train->coaches[i].totalSeats; maxRac = train->coaches[i].racCapacity; break;
        }
    }
    if (maxSeats < 0 || maxRac < 0) { printf("Cancellation stopped: train/class capacity was not found.\n"); return; }

    ReallocationResult result;
    if (!reallocateAfterCancellation(booking, inventory, maxSeats, maxRac, &result)) {
        printf("Cancellation stopped: invalid ticket status or inventory already at capacity.\n"); return;
    }
    if (saveBookingData() != SUCCESS || saveAvailabilityData() != SUCCESS) {
        rollbackReallocation(booking, inventory, &result);
        saveBookingData(); saveAvailabilityData();
        printf("Cancellation failed while saving. Changes were rolled back where possible. No refund is recorded.\n"); return;
    }
    printf("\nTicket cancelled successfully.\nFare paid                    : Rs. %.2f\nCancellation deduction (25%%) : Rs. %.2f\nRefund amount (75%%)           : Rs. %.2f\n",
           booking->fare, booking->fare * 0.25f, refund);
    printf("Note: This project calculates the refund; it does not transfer money through a real payment gateway.\n");
    if (result.promotedToCNF) printf("Passenger PNR %s promoted to CNF.\n", result.promotedToCNF->pnrNumber);
    if (result.promotedToRAC) printf("Passenger PNR %s promoted to RAC.\n", result.promotedToRAC->pnrNumber);
    if (!result.promotedToCNF && !result.promotedToRAC) printf("No queued passenger needed promotion.\n");
}
