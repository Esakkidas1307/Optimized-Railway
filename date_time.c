#define _POSIX_C_SOURCE 200809L
#include "date_time.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

/* Force all project timestamps and date comparisons to Indian Standard Time. */
static void ensure_ist_timezone(void) {
    static int initialized = 0;
    if (!initialized) {
        if (setenv("TZ", "Asia/Kolkata", 1) == 0) tzset();
        initialized = 1;
    }
}

static int parse_date(const char *date, int *d, int *m, int *y) {
    char extra;
    if (!date || strlen(date)!=10 || date[2]!=':' || date[5]!=':' || sscanf(date,"%2d:%2d:%4d%c",d,m,y,&extra)!=3 || !isValidJourneyDate(date)) return 0;
    return 1;
}
static int parse_time(const char *s, int *h, int *m) {
    char extra;
    if (!s || strlen(s)!=5 || s[2]!=':' || sscanf(s,"%2d:%2d%c",h,m,&extra)!=2 || *h<0 || *h>23 || *m<0 || *m>59) return 0;
    return 1;
}
int railway_parse_datetime(const char *date,const char *hhmm,time_t *out) {
    ensure_ist_timezone();
    int d,m,y,h,mi; struct tm t;
    if(!parse_date(date,&d,&m,&y)||!parse_time(hhmm,&h,&mi)||!out) return 0;
    memset(&t,0,sizeof t); t.tm_mday=d;t.tm_mon=m-1;t.tm_year=y-1900;t.tm_hour=h;t.tm_min=mi;t.tm_isdst=-1;
    *out=mktime(&t); return *out!=(time_t)-1;
}
int railway_date_is_future(const char *date) {
    ensure_ist_timezone();
    int d,m,y; struct tm t; time_t now=time(NULL); if(!parse_date(date,&d,&m,&y))return 0;
    localtime_r(&now,&t); t.tm_hour=0;t.tm_min=0;t.tm_sec=0;t.tm_isdst=-1;
    time_t today=mktime(&t); memset(&t,0,sizeof t);t.tm_mday=d;t.tm_mon=m-1;t.tm_year=y-1900;t.tm_isdst=-1;
    return mktime(&t)>today;
}
int railway_schedule_lead_time_ok(const char *date, int type) {
    int day;
    int month;
    int year;
    struct tm today_tm;
    struct tm requested_tm;
    time_t now;
    time_t today;
    time_t requested;
    time_t earliest;

    ensure_ist_timezone();
    if ((type != 1) && (type != 2)) {
        return 0;
    }
    if (!parse_date(date, &day, &month, &year)) {
        return 0;
    }

    now = time(NULL);
    if (localtime_r(&now, &today_tm) == NULL) {
        return 0;
    }
    today_tm.tm_hour = 0;
    today_tm.tm_min = 0;
    today_tm.tm_sec = 0;
    today_tm.tm_isdst = -1;
    today = mktime(&today_tm);
    if (today == (time_t)-1) {
        return 0;
    }

    if (type == 1) {
        /* Add one calendar month, clamping month-end dates when necessary. */
        struct tm target_tm = today_tm;
        int original_day = today_tm.tm_mday;
        int target_month;
        int target_year;
        int target_last_day;
        static const int month_days[12] =
            {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        target_tm.tm_mday = 1;
        target_tm.tm_mon += 1;
        earliest = mktime(&target_tm);
        if (earliest == (time_t)-1 || localtime_r(&earliest, &target_tm) == NULL) {
            return 0;
        }
        target_month = target_tm.tm_mon;
        target_year = target_tm.tm_year + 1900;
        target_last_day = month_days[target_month];
        if ((target_month == 1) &&
            (((target_year % 400) == 0) ||
             (((target_year % 4) == 0) && ((target_year % 100) != 0)))) {
            target_last_day = 29;
        }
        target_tm.tm_mday = (original_day < target_last_day) ? original_day : target_last_day;
        target_tm.tm_hour = 0;
        target_tm.tm_min = 0;
        target_tm.tm_sec = 0;
        target_tm.tm_isdst = -1;
        earliest = mktime(&target_tm);
        if (earliest == (time_t)-1) {
            return 0;
        }
    } else {
        /* Special trains must be scheduled at least seven calendar days ahead. */
        today_tm.tm_mday += 7;
        earliest = mktime(&today_tm);
        if (earliest == (time_t)-1) {
            return 0;
        }
    }

    memset(&requested_tm, 0, sizeof(requested_tm));
    requested_tm.tm_mday = day;
    requested_tm.tm_mon = month - 1;
    requested_tm.tm_year = year - 1900;
    requested_tm.tm_isdst = -1;
    requested = mktime(&requested_tm);
    if (requested == (time_t)-1) {
        return 0;
    }
    return requested >= earliest;
}

int railway_reserved_cutoff_ok(const char *date,const char *departure_hhmm) {
    ensure_ist_timezone();
    time_t dep; if(!railway_parse_datetime(date,departure_hhmm,&dep))return 0;
    return dep-time(NULL)>=24*60*60;
}
int railway_departure_is_future_today(const char *date,const char *departure_hhmm) {
    ensure_ist_timezone();
    time_t dep; struct tm a,b; time_t now=time(NULL);
    if(!railway_parse_datetime(date,departure_hhmm,&dep)||dep<=now)return 0;
    localtime_r(&now,&a);localtime_r(&dep,&b);
    return a.tm_year==b.tm_year && a.tm_yday==b.tm_yday;
}
void railway_timestamp(char *out,unsigned long size) {
    ensure_ist_timezone();
    time_t now=time(NULL); struct tm t; if(!out||size==0)return;
    localtime_r(&now,&t); if(!strftime(out,size,"%d:%m:%Y %H:%M:%S",&t))snprintf(out,size,"UNKNOWN");
}
void railway_log_event(const char *file,const char *username,const char *event) {
    FILE *fp=fopen(file,"a"); char stamp[32]; if(!fp)return; railway_timestamp(stamp,sizeof stamp);
    fprintf(fp,"%s | %s | %s\n",stamp,username&&*username?username:"unknown",event?event:"EVENT"); fclose(fp);
}
void railway_print_current_time(void) { char stamp[32]; railway_timestamp(stamp,sizeof stamp); printf("Current system time: %s\n",stamp); }
