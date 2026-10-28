#ifndef AVAILABILITY_H
#define AVAILABILITY_H

#include "common.h"
#include "utils.h"

typedef struct Availability
{
    int trainNumber;

    char journeyDate[11];

    char className[MAX_CLASS_LEN];

    int availableSeats;

    int availableRAC;

    int availableWL;

    struct Availability *prev;
    struct Availability *next;

} Availability;

extern Availability *availabilityHead;

void initializeAvailabilityModule(void);

void loadAvailabilityData(void);

int saveAvailabilityData(void);

void createAvailabilityRecord(
    Train *train,
    const char *journeyDate);

Availability *findAvailability(
    int trainNumber,
    const char *journeyDate,
    const char *className);
    
void checkAvailability(void);



#endif
