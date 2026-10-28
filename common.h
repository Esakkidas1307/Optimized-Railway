#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"

typedef enum
{
    CONFIRMED = CNF,
    RAC_STATUS = RAC,
    WL_STATUS = WL,
    CANCELLED_STATUS = CANCELLED

} TicketStatus;
/* Station */

typedef struct
{
    char stationName[MAX_STATION_LEN];

    char arrivalTime[10];
    char departureTime[10];

    int distanceFromSource;

} Station;


/* Coach */

typedef struct
{
    char className[MAX_CLASS_LEN];

    int noOfCoaches;

    int seatsPerCoach;

    int totalSeats;

    int availableSeats;

    int racCapacity;
    int availableRAC;

    int wlCapacity;
    int availableWL;

    float farePerKM;

} Coach;


/* Train */

typedef struct Train
{
    int trainNumber;

    char trainName[MAX_NAME_LEN];

    int totalStations;

    Station stations[MAX_STATIONS];

    int classCount;

    Coach coaches[MAX_COACHES];

    int isActive;

    struct Train *prev;
    struct Train *next;

} Train;

typedef struct TrainSchedule
{
    int trainNumber;

    char journeyDate[11];

    int status;

    int delayMinutes;

    struct TrainSchedule *prev;
    struct TrainSchedule *next;

} TrainSchedule;
#endif
