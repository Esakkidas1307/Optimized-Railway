#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "date_time.h"
#include "admin_auth.h"

#define ADMIN_FILE "admin.txt"
#define MAX_CREDENTIAL_LEN 99

static int readCredentialLine(FILE *fp, char *out, size_t size)
{
    if (!fgets(out, (int)size, fp)) return 0;
    size_t n = strlen(out);
    if (n && out[n-1] == '\n') out[--n] = '\0';
    else if (!feof(fp)) return 0;
    if (n && out[n-1] == '\r') out[--n] = '\0';
    return 1;
}

int adminLogin(void)
{
    FILE *fp = fopen(ADMIN_FILE, "r");
    char marker[32], storedUser[MAX_CREDENTIAL_LEN+1], storedPass[MAX_CREDENTIAL_LEN+1], end[32];
    int attempt;
    if (!fp) {
        printf("Admin access denied: admin.txt is missing. Developer must create it manually.\n");
        return FAILURE;
    }
    int valid = readCredentialLine(fp, marker, sizeof marker) &&
                readCredentialLine(fp, storedUser, sizeof storedUser) &&
                readCredentialLine(fp, storedPass, sizeof storedPass) &&
                readCredentialLine(fp, end, sizeof end) &&
                strcmp(marker, "ADMIN_START") == 0 &&
                strcmp(end, "ADMIN_END") == 0 &&
                storedUser[0] != '\0' && storedPass[0] != '\0';
    if (valid) {
        int ch;
        while ((ch = fgetc(fp)) != EOF) {
            if (ch != '\n' && ch != '\r' && ch != ' ' && ch != '\t') { valid = 0; break; }
        }
    }
    fclose(fp);
    if (!valid) {
        printf("Admin access denied: admin.txt has an invalid format.\n");
        return FAILURE;
    }
    for (attempt = 1; attempt <= 3; ++attempt) {
        char username[MAX_CREDENTIAL_LEN+1], password[MAX_CREDENTIAL_LEN+1];
        getStringInput("Admin username: ", username, sizeof username);
        getStringInput("Admin password: ", password, sizeof password);
        if (strcmp(username, storedUser) == 0 && strcmp(password, storedPass) == 0) {
            printf("Admin login successful.\n");
            railway_log_event("adminlog.txt", storedUser, "LOGIN");
            return SUCCESS;
        }
        printf("Invalid admin credentials (%d/3).\n", attempt);
    }
    printf("Maximum admin login attempts reached. Returning to main menu.\n");
    return FAILURE;
}
