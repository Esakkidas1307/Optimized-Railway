#include <setjmp.h>

#include "passenger.h"
#include "utils.h"
#include "user.h"

void railway_set_navigation_target(jmp_buf *target)
{
    (void)target;
}

Passenger *passengerHead = NULL;
User *userHead = NULL;

int isValidJourneyDate(const char *date)
{
    (void)date;
    return 0;
}
