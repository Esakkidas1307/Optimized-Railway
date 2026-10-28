#include "utils.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <setjmp.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>
#include "optimization_runtime.h"

static jmp_buf *navigation_target = NULL;
static void navigate_after_input_failure(void);
static char last_prompt[256];
static int repeated_prompt_count = 0;
static void track_prompt(const char *message) {
    const char *m = message ? message : "";
    /* Menu choices have their own explicit retry policy in main/usermenu/adminmenu. */
    if (strstr(m, "choice") || strstr(m, "Choice") || strstr(m, "CHOICE")) {
        last_prompt[0] = '\0'; repeated_prompt_count = 0; return;
    }
    if (strncmp(last_prompt, m, sizeof last_prompt) == 0) repeated_prompt_count++;
    else { snprintf(last_prompt, sizeof last_prompt, "%s", m); repeated_prompt_count = 1; }
    /* A semantic validation loop has repeated the same prompt three times already. */
    if (repeated_prompt_count >= 4) {
        printf("Three invalid values for this field. Returning to the active menu.\n");
        repeated_prompt_count = 0; last_prompt[0] = '\0'; navigate_after_input_failure();
    }
}
void railway_set_navigation_target(jmp_buf *target) { navigation_target = target; }
static void navigate_after_input_failure(void) { if (navigation_target) longjmp(*navigation_target, 1); }

void initializeSystem(void)
{
    (void)railway_runtime_init(30U);
    printf("\n======================================\n");
    printf(" Railway Reservation System\n");
    printf("======================================\n");
}

void cleanupSystem(void)
{
    railway_runtime_shutdown();
    printf("\nSystem Closed Successfully...\n");
}


static int readLine(const char *message, char *buffer, size_t size)
{
    size_t len;
    int ch;
    if (!buffer || size < 2) return FAILURE;
    track_prompt(message);
    printf("%s", message);
    fflush(stdout);
    {
        unsigned elapsed_seconds = 0U;
        struct pollfd input_descriptor;
        input_descriptor.fd = STDIN_FILENO;
        input_descriptor.events = POLLIN;
        input_descriptor.revents = 0;
        while (elapsed_seconds < 300U) {
            int poll_result = poll(&input_descriptor, 1U, 1000);
            railway_runtime_service();
            if (railway_runtime_interruption_requested() != 0) {
                railway_runtime_request_timeout();
                navigate_after_input_failure();
                return FAILURE;
            }
            if (poll_result > 0) {
                if ((input_descriptor.revents & (POLLIN | POLLHUP)) != 0) {
                    break;
                }
                buffer[0] = '\0';
                return FAILURE;
            }
            if (poll_result < 0) {
                if (errno == EINTR) continue;
                buffer[0] = '\0';
                return FAILURE;
            }
            ++elapsed_seconds;
        }
        if (elapsed_seconds >= 300U) {
            buffer[0] = '\0';
            railway_runtime_request_timeout();
            navigate_after_input_failure();
            return FAILURE;
        }
    }
    if (!fgets(buffer, (int)size, stdin)) {
        buffer[0] = '\0';
        return FAILURE;
    }
    len = strlen(buffer);
    if (len && buffer[len - 1] == '\n') {
        buffer[--len] = '\0';
        if (len && buffer[len - 1] == '\r') buffer[--len] = '\0';
        return SUCCESS;
    }
    ch = getchar();
    if (ch != EOF && ch != '\n') {
        while (ch != '\n' && ch != EOF) ch = getchar();
        buffer[0] = '\0';
        printf("Input too long. Please enter no more than %lu characters.\n", (unsigned long)(size - 1));
        return FAILURE;
    }
    return SUCCESS;
}

int getIntegerInput(const char *message)
{
    char buffer[128], *end;
    long value;
    for (int attempt = 1; attempt <= 3; ++attempt) {
        if (!readLine(message, buffer, sizeof buffer)) {
            if (feof(stdin)) return 0;
            printf("Invalid input (%d/3).\n", attempt); continue;
        }
        char *p = buffer;
        while (isspace((unsigned char)*p)) p++;
        if (*p == '\0') { printf("Input cannot be empty (%d/3).\n", attempt); continue; }
        errno = 0; value = strtol(p, &end, 10);
        while (isspace((unsigned char)*end)) end++;
        if (errno == ERANGE || end == p || *end != '\0' || value < INT_MIN || value > INT_MAX) {
            printf("Invalid whole number (%d/3).\n", attempt); continue;
        }
        return (int)value;
    }
    printf("Three invalid attempts. Returning to the active menu.\n");
    navigate_after_input_failure();
    return 0;
}

void getStringInput(const char *message, char *buffer, int size)
{
    if (!buffer || size <= 0) return;
    for (int attempt = 1; attempt <= 3; ++attempt) {
        if (!readLine(message, buffer, (size_t)size)) {
            if (feof(stdin)) { buffer[0] = '\0'; return; }
            printf("Invalid input (%d/3).\n", attempt); continue;
        }
        char *p = buffer;
        while (isspace((unsigned char)*p)) p++;
        if (*p == '\0') { printf("Input cannot be empty (%d/3).\n", attempt); continue; }
        if (p != buffer) memmove(buffer, p, strlen(p) + 1);
        return;
    }
    buffer[0] = '\0';
    printf("Three invalid attempts. Returning to the active menu.\n");
    navigate_after_input_failure();
}

float getFloatInput(const char *message)
{
    char buffer[128], *end;
    double value;
    for (int attempt = 1; attempt <= 3; ++attempt) {
        if (!readLine(message, buffer, sizeof buffer)) {
            if (feof(stdin)) return -1.0f;
            printf("Invalid amount (%d/3).\n", attempt); continue;
        }
        char *p = buffer;
        while (isspace((unsigned char)*p)) p++;
        if (*p == '\0') { printf("Amount cannot be empty (%d/3).\n", attempt); continue; }
        errno = 0; value = strtod(p, &end);
        while (isspace((unsigned char)*end)) end++;
        if (errno == ERANGE || end == p || *end != '\0' || !isfinite(value) || value > 1.0e9 || value < -1.0e9) {
            printf("Invalid amount (%d/3).\n", attempt); continue;
        }
        return (float)value;
    }
    printf("Three invalid attempts. Returning to the active menu.\n");
    navigate_after_input_failure();
    return -1.0f;
}

int isValidJourneyDate(const char *date)
{
    int dd, mm, yyyy;
    if (!date || strlen(date) != 10 || date[2] != ':' || date[5] != ':') return FAILURE;
    for (int i = 0; i < 10; ++i)
        if (i != 2 && i != 5 && (date[i] < '0' || date[i] > '9')) return FAILURE;
    if (sscanf(date, "%2d:%2d:%4d", &dd, &mm, &yyyy) != 3 || yyyy < 1900 || yyyy > 9999 || mm < 1 || mm > 12) return FAILURE;
    static const int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int maxDay = days[mm - 1];
    if (mm == 2 && ((yyyy % 400 == 0) || (yyyy % 4 == 0 && yyyy % 100 != 0))) maxDay = 29;
    return dd >= 1 && dd <= maxDay ? SUCCESS : FAILURE;
}

void clearInputBuffer(void)
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }
}
