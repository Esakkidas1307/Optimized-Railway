#define _POSIX_C_SOURCE 200809L
#include "unreserved_booking.h"
#include "reserved_booking.h"
#include "train.h"
#include "trainschedule.h"
#include "utils.h"
#include "availability.h"
#include "date_time.h"
#include <ctype.h>

static int findStation(const Train *t, const char *name)
{
    for (int i = 0; i < t->totalStations; ++i)
        if (strcmp(t->stations[i].stationName, name) == 0) return i;
    return -1;
}

static int routeDistance(const Train *t, int from, int to)
{
    int d = t->stations[to].distanceFromSource - t->stations[from].distanceFromSource;
    return d < 0 ? -d : d;
}

static int hasActiveSchedule(int trainNo, const char *date, const char *source, const Train *train)
{
    int from = findStation(train, source);
    if (from < 0 || !railway_departure_is_future_today(date, train->stations[from].departureTime)) return 0;
    for (TrainSchedule *s = scheduleHead; s; s = s->next)
        if (s->trainNumber == trainNo && strcmp(s->journeyDate, date) == 0 && s->status != TRAIN_CANCELLED) return 1;
    return 0;
}

void bookUnreservedTicket(int passengerId)
{
    char source[MAX_STATION_LEN], destination[MAX_STATION_LEN], today[11];
    time_t now = time(NULL); struct tm localNow; localtime_r(&now, &localNow);
    strftime(today, sizeof today, "%d:%m:%Y", &localNow);
    
    for (;;) {
        int matchCount = 0;
        getStringInput("Source station (enter 0 to cancel): ", source, sizeof source);
        if (strcmp(source, "0") == 0) return;
        getStringInput("Destination station (enter 0 to cancel): ", destination, sizeof destination);
        if (strcmp(destination, "0") == 0) return;
        if (strcmp(source, destination) == 0) {
            printf("Source and destination must differ.\n");
            return;
        }
        matchCount = 0;
        printf("\nScheduled trains serving this route:\n");
        for (Train *t = trainHead; t; t = t->next) {
            int from = findStation(t, source), to = findStation(t, destination);
            if (from >= 0 && to > from && hasActiveSchedule(t->trainNumber, today, source, t)) {
                printf("Train %d - %s | Distance: %d km\n", t->trainNumber, t->trainName, routeDistance(t, from, to));
                ++matchCount;
            }
        }
        if (matchCount > 0) break;
        printf("No future train found for this route today. Returning to User Menu.\n");
        return;
    }

    int typeChoice;
    for (int tries = 1; tries <= 3; ++tries) {
        typeChoice = getIntegerInput("\nTrain type (1. EXP, 2. SUPERFAST; 0 to cancel): ");
        if (typeChoice == 0) return;
        if (typeChoice == 1 || typeChoice == 2) break;
        printf("Choose 1 for EXP or 2 for SUPERFAST (%d/3).\n", tries);
        if (tries == 3) return;
    }

    int from, to;
    const Train *train = NULL;
    from = -1;
    to = -1;
    for (int tries = 1; tries <= 3; ++tries) {
        int trainNo = getIntegerInput("Select train number from the list above (0 to cancel): ");
        if (trainNo == 0) return;
        train = findTrain(trainNo);
        from = train ? findStation(train, source) : -1;
        to = train ? findStation(train, destination) : -1;
        if (train && from >= 0 && to > from && hasActiveSchedule(trainNo, today, source, train)) break;
        train = NULL;
        printf("Choose a train number from the displayed route list (%d/3).\n", tries);
        if (tries == 3) return;
    }

    int adults, children;
    adults = children = -1;
    for (int tries = 1; tries <= 3; ++tries) {
        adults = getIntegerInput("Number of adults (0 or more, e.g., 2): ");
        children = getIntegerInput("Number of children (0 or more, e.g., 1): ");
        if (adults >= 0 && children >= 0 && adults + children >= 1 && adults <= 100 && children <= 100 && adults + children <= 100) break;
        printf("Invalid passenger counts; total must be 1-100 (%d/3).\n", tries);
        if (tries == 3) return;
    }

    int distance = routeDistance(train, from, to);
    float adultRate = typeChoice == 2 ? 0.60f : 0.50f;
    float childRate = typeChoice == 2 ? 0.15f : 0.10f;
    float fare = (float)distance * ((float)adults * adultRate + (float)children * childRate);
    const char *typeName = typeChoice == 2 ? "SUPERFAST" : "EXP";

    printf("\n============== FARE DETAILS ==============\n");
    printf("Train             : %d - %s (%s)\n", train->trainNumber, train->trainName, typeName);
    printf("Route             : %s -> %s\n", source, destination);
    printf("Distance          : %d km\n", distance);
    printf("Adults            : %d x Rs. %.2f/km\n", adults, adultRate);
    printf("Children          : %d x Rs. %.2f/km\n", children, childRate);
    printf("Total fare        : Rs. %.2f\n", fare);
    printf("==========================================\n");

    int paid = 0;
    for (int tries = 1; tries <= 3; ++tries) {
        float amount = getFloatInput("Enter exact fare amount to pay in rupees (e.g., 125.50): ");
        if (amount < 0.0f) { printf("Invalid payment input exhausted its three input attempts. Returning to User Menu.\n"); return; }
        if (amount >= fare - 0.005f && amount <= fare + 0.005f) { paid = 1; break; }
        printf("Payment declined. Enter Rs. %.2f exactly (attempt %d of 3).\n", fare, tries);
    }
    if (!paid) {
        printf("Payment failed. No unreserved ticket was created.\n");
        return;
    }

    Booking *b = calloc(1, sizeof *b);
    if (!b) { printf("Memory allocation failed. Ticket not created.\n"); return; }
    b->pnrNumber[0] = '\0';
    b->passengerId = passengerId;
    snprintf(b->passengerName, sizeof b->passengerName, "Unreserved passenger(s)");
    snprintf(b->gender, sizeof b->gender, "NA");
    b->age = 0;
    b->trainNumber = train->trainNumber;
    snprintf(b->trainName, sizeof b->trainName, "%.35s %.9s", train->trainName, typeName);
    snprintf(b->source, sizeof b->source, "%s", source);
    snprintf(b->destination, sizeof b->destination, "%s", destination);
    snprintf(b->journeyDate, sizeof b->journeyDate, "%s", today);
    b->distance = distance;
    snprintf(b->className, sizeof b->className, "UNRES");
    b->fare = fare;
    b->ticketStatus = CNF;
    snprintf(b->bookingType, sizeof b->bookingType, "UNRESERVED");
    b->adults = adults;
    b->children = children;
    railway_timestamp(b->bookedAt, sizeof b->bookedAt);

    b->next = bookingHead;
    if (bookingHead) bookingHead->prev = b;
    bookingHead = b;
    if (saveBookingData() != SUCCESS) {
        bookingHead = b->next;
        if (bookingHead) bookingHead->prev = NULL;
        free(b);
        printf("Could not save booking record. No ticket issued.\n");
        return;
    }
    printf("\nPayment successful. Unreserved ticket generated.\n");
    printBookingTicket(b);
}
