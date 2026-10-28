#include "reallocation.h"

static Booking *oldestQueuedBooking(int trainNo, const char *date,
                                    const char *className, int wantedStatus,
                                    const Booking *exclude)
{
    Booking *chosen = NULL;
    for (Booking *b = bookingHead; b != NULL; b = b->next) {
        if (b == exclude || strcmp(b->bookingType, "UNRESERVED") == 0 || b->ticketStatus != wantedStatus || b->trainNumber != trainNo ||
            strcmp(b->journeyDate, date) != 0 || strcmp(b->className, className) != 0) continue;
        if (chosen == NULL || strcmp(b->pnrNumber, chosen->pnrNumber) < 0) chosen = b;
    }
    return chosen;
}

int reallocateAfterCancellation(Booking *booking, Availability *inventory,
                                int maxSeats, int maxRac, ReallocationResult *r)
{
    if (!booking || !inventory || !r) return 0;
    r->oldTicketStatus = booking->ticketStatus;
    r->oldAvailableSeats = inventory->availableSeats;
    r->oldAvailableRAC = inventory->availableRAC;
    r->oldAvailableWL = inventory->availableWL;
    r->promotedToCNF = NULL;
    r->promotedToRAC = NULL;
    r->promotedToCNFOldStatus = 0;

    if (booking->ticketStatus == CNF) {
        if (inventory->availableSeats >= maxSeats) return 0;
        inventory->availableSeats++;
    } else if (booking->ticketStatus == RAC) {
        if (inventory->availableRAC >= maxRac) return 0;
        inventory->availableRAC++;
    } else if (booking->ticketStatus != WL) return 0;

    booking->ticketStatus = CANCELLED;
    if (r->oldTicketStatus == CNF && inventory->availableSeats > 0) {
        r->promotedToCNF = oldestQueuedBooking(booking->trainNumber, booking->journeyDate,
                                                booking->className, RAC, booking);
        if (r->promotedToCNF) {
            r->promotedToCNFOldStatus = RAC;
            r->promotedToCNF->ticketStatus = CNF;
            inventory->availableSeats--;
            if (inventory->availableRAC < maxRac) inventory->availableRAC++;
        } else {
            r->promotedToCNF = oldestQueuedBooking(booking->trainNumber, booking->journeyDate,
                                                    booking->className, WL, booking);
            if (r->promotedToCNF) {
                r->promotedToCNFOldStatus = WL;
                r->promotedToCNF->ticketStatus = CNF;
                inventory->availableSeats--;
            }
        }
    }
    if (inventory->availableRAC > 0) {
        r->promotedToRAC = oldestQueuedBooking(booking->trainNumber, booking->journeyDate,
                                                booking->className, WL, booking);
        if (r->promotedToRAC) {
            r->promotedToRAC->ticketStatus = RAC;
            inventory->availableRAC--;
        }
    }
    return 1;
}

void rollbackReallocation(Booking *booking, Availability *inventory, const ReallocationResult *r)
{
    if (!booking || !inventory || !r) return;
    booking->ticketStatus = r->oldTicketStatus;
    if (r->promotedToCNF) r->promotedToCNF->ticketStatus = r->promotedToCNFOldStatus;
    if (r->promotedToRAC) r->promotedToRAC->ticketStatus = WL;
    inventory->availableSeats = r->oldAvailableSeats;
    inventory->availableRAC = r->oldAvailableRAC;
    inventory->availableWL = r->oldAvailableWL;
}
