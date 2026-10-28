#ifndef CONSTANTS_H
#define CONSTANTS_H

/* General Limits */

#define MAX_NAME_LEN          50
#define MAX_STATION_LEN       50
#define MAX_CLASS_LEN         10

#define MAX_TRAINS           100
#define MAX_STATIONS          20
#define MAX_COACHES           10
#define MAX_PASSENGERS       500

#define SUCCESS               1
#define FAILURE               0

/* Ticket Status */

#define CNF                   1
#define RAC                   2
#define WL                    3
#define CANCELLED             4

/* Menu Options */

#define RESERVED_BOOKING      1
#define UNRESERVED_BOOKING    2
#define PNR_STATUS            3
#define TICKET_CANCELLATION   4
#define EXIT_APPLICATION      0

#define ACTIVE      1
#define INACTIVE    0
#define MAX_TIME_LEN 6

#define MAX_PHONE_LEN 15
#define MAX_GENDER_LEN 10

#define TRAIN_RUNNING       1
#define TRAIN_RESCHEDULED   2
#define TRAIN_CANCELLED     3
#define TRAIN_SPECIAL       4
#endif
