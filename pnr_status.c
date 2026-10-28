#include "pnr_status.h"
#include "reserved_booking.h"
#include "utils.h"
#include "date_time.h"

void verifyPNRStatus(void)
{
    char pnr[20];
    getStringInput("Enter reserved-ticket PNR: ", pnr, sizeof pnr);

    for (Booking *b = bookingHead; b != NULL; b = b->next) {
        if (strcmp(b->bookingType, "UNRESERVED") != 0 && b->pnrNumber[0] != '\0' && strcmp(b->pnrNumber, pnr) == 0) {
            printf("\nPNR verified successfully. Ticket report follows.\n");
            railway_print_current_time();
            printBookingTicket(b);
            return;
        }
    }

    printf("\nInvalid PNR: %s was not found. Returning to Main Menu.\n", pnr);
}
