#ifndef BOOKING_H
#define BOOKING_H
#include "common.h"


#ifdef __cplusplus
extern "C" {
#endif
typedef struct Booking {
    char pnrNumber[20];
    int passengerId;
    char passengerName[MAX_NAME_LEN];
    int age;
    char gender[MAX_GENDER_LEN];
    int trainNumber;
    char trainName[MAX_NAME_LEN];
    char source[MAX_STATION_LEN];
    char destination[MAX_STATION_LEN];
    char journeyDate[11];
    int distance;
    char className[MAX_CLASS_LEN];
    float fare;
    int ticketStatus;
    char bookingType[20]; /* RESERVED or UNRESERVED */
    int adults;
    int children;
    char bookedAt[32];
    struct Booking *prev, *next;
} Booking;
extern Booking *bookingHead;
void initializeBookingModule(void);
void loadBookingData(void);
int saveBookingData(void);
void bookTicket(int passengerId); /* reserved booking */
void generateBookingPNR(char out[20]);
void printBookingTicket(const Booking *booking);
void viewMyBookings(int passengerId);
void cancelTicket(int passengerId);

#ifdef __cplusplus
}
#endif

#endif
