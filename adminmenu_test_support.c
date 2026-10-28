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
