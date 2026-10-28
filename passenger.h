#ifndef PASSENGER_H
#define PASSENGER_H

#include "common.h"

typedef struct Passenger
{
    int passengerId;

    char passengerName[MAX_NAME_LEN];

    int age;

    char gender[10];

    char phoneNumber[15];

    struct Passenger *prev;
    struct Passenger *next;

} Passenger;

extern Passenger *passengerHead;

void initializePassengerModule(void);

void loadPassengerData(void);

int savePassengerData(void);

void viewMyProfile(int passengerId);

void updateMyProfile(int passengerId);

void createPassengerProfile(int passengerId);

void viewPassengers(void);

void searchPassenger(void);

void updatePassenger(void);

void deletePassenger(void);

#endif
