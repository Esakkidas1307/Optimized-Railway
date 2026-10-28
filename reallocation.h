#ifndef REALLOCATION_H
#define REALLOCATION_H
#include "reserved_booking.h"
#include "availability.h"
typedef struct {
    int oldTicketStatus;
    int oldAvailableSeats;
    int oldAvailableRAC;
    int oldAvailableWL;
    Booking *promotedToCNF;
    Booking *promotedToRAC;
    int promotedToCNFOldStatus;
} ReallocationResult;
int reallocateAfterCancellation(Booking *booking, Availability *const inventory,
                                int maxSeats, int maxRac, ReallocationResult *r);
void rollbackReallocation(Booking *booking, Availability *const inventory, const ReallocationResult *r);


#endif
