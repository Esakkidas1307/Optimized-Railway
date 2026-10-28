#include <ctype.h>
#include <stdio.h>
#include <string.h>

int isValidJourneyDate(const char *date)
{
    int day, month, year;
    char extra;

    if (!date || strlen(date) != 10 ||
        date[2] != ':' || date[5] != ':')
        return 0;

    if (sscanf(date, "%2d:%2d:%4d%c",
               &day, &month, &year, &extra) != 3)
        return 0;

    if (month < 1 || month > 12 || day < 1 || year < 1)
        return 0;

    const int daysInMonth[] = {
        31, (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 29 : 28,
        31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    return day <= daysInMonth[month - 1];
}
