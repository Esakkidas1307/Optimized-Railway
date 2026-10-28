#include <setjmp.h>
#include "utils.h"
#include "date_time.h"

void railway_set_navigation_target(jmp_buf *target)
{
    (void)target;
}

void railway_log_event(const char *file, const char *username, const char *event)
{
    (void)file;
    (void)username;
    (void)event;
}

/* Persistence callbacks referenced by main.c; application tests use stubs. */
int saveTrainData(void) { return 1; }
int saveTrainScheduleData(void) { return 1; }
int savePassengerData(void) { return 1; }
int saveAvailabilityData(void) { return 1; }
int saveUserData(void) { return 1; }
int saveBookingData(void) { return 1; }
