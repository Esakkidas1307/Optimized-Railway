#ifndef DATE_TIME_H
#define DATE_TIME_H
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif


int railway_parse_datetime(const char *date, const char *hhmm, time_t *out);
int railway_date_is_future(const char *date);
/* type: 1 = regular/running, 2 = special */
int railway_schedule_lead_time_ok(const char *date, int type);
int railway_reserved_cutoff_ok(const char *date, const char *departure_hhmm);
int railway_departure_is_future_today(const char *date, const char *departure_hhmm);
void railway_timestamp(char *out, unsigned long size);
void railway_log_event(const char *file, const char *username, const char *event);
void railway_print_current_time(void);

#ifdef __cplusplus
}
#endif


#endif
