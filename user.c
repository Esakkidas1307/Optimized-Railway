#include "user.h"
#include "passenger.h"
#include "utils.h"
#include "date_time.h"
#include "optimization_runtime.h"

#define USER_FILE "users.txt"
#define USER_TEMP_FILE "users.txt.tmp"

User *userHead = NULL;

static int generatePassengerId(void);

    
static int userExists(int passengerId);


static int usernameExists(const char *username);

static int generatePassengerId(void)
{
    int maxId = 100;

    User *current =
        userHead;

    while(current != NULL)
    {
        if(current->passengerId >
           maxId)
        {
            maxId =
            current->passengerId;
        }

        current =
            current->next;
    }

    return maxId + 1;
}


void initializeUserModule(void)
{
    FILE *fp;

    fp = fopen(USER_FILE, "a");

    if(fp == NULL)
    {
        printf(
        "\nUnable To Create User File\n");

        return;
    }

    fclose(fp);

    loadUserData();
}
    


static int usernameExists(
    const char *username)
{
    User *current =
        userHead;

    while(current != NULL)
    {
        if(strcmp(
           current->username,
           username) == 0)
        {
            return SUCCESS;
        }

        current =
            current->next;
    }

    return FAILURE;
}

int registerUser(void)
{
    User *newUser;

    newUser =
    (User *)malloc(
    sizeof(User));

    if(newUser == NULL)
    {
        printf(
        "\nMemory Allocation Failed\n");

        return FAILURE ;
    }

    newUser->passengerId =generatePassengerId();

    printf("\nGenerated Passenger ID : %d\n",newUser->passengerId);

    while(1)
    {
        getStringInput(
        "Username (up to 99 characters): ",
        newUser->username,
        sizeof newUser->username);

        if(!usernameExists(
            newUser->username))
        {
            break;
        }

        printf("Username Already Exists.\n");
    }

    getStringInput(
    "Password (non-empty; up to 99 characters): ",
    newUser->password,
    sizeof newUser->password);

    newUser->prev = NULL;

    newUser->next =
        userHead;

    if(userHead != NULL)
    {
        userHead->prev =newUser;
    }

    userHead =newUser;

    saveUserData();

    printf("\nRegistration Successful\n");
    railway_log_event("userlog.txt", newUser->username, "REGISTER");

    return newUser->passengerId;

}

int loginUser(void)
{
    char username[100];
    char password[100];
    int attempt;

    for (attempt = 1; attempt <= 3; ++attempt)
    {
        User *current = userHead;
        getStringInput("\nUsername: ", username, sizeof username);
        getStringInput("Password: ", password, sizeof password);
        while (current != NULL)
        {
            if (strcmp(current->username, username) == 0 &&
                strcmp(current->password, password) == 0)
            {
                printf("\nLogin Successful\n");
                railway_log_event("userlog.txt", current->username, "LOGIN");
                return current->passengerId;
            }
            current = current->next;
        }
        printf("Invalid username or password (%d/3).\n", attempt);
    }
    printf("Maximum login attempts reached. Returning to main menu.\n");
    return FAILURE;
}

int saveUserData(void)
{
    FILE *fp;
    User *current;
    railway_runtime_mark_dirty(RAILWAY_DATA_USERS);
    fp = fopen(USER_TEMP_FILE, "w");
    if (fp == NULL) return FAILURE;
    for (current = userHead; current != NULL; current = current->next) {
        if (fprintf(fp, "USER_START\n%d\n%s\n%s\nUSER_END\n",
                    current->passengerId, current->username, current->password) < 0) {
            (void)fclose(fp);
            (void)remove(USER_TEMP_FILE);
            return FAILURE;
        }
    }
    if (fflush(fp) != 0 || ferror(fp)) {
        (void)fclose(fp);
        (void)remove(USER_TEMP_FILE);
        return FAILURE;
    }
    if (fclose(fp) != 0) {
        (void)remove(USER_TEMP_FILE);
        return FAILURE;
    }
    if (rename(USER_TEMP_FILE, USER_FILE) != 0) {
        (void)remove(USER_TEMP_FILE);
        return FAILURE;
    }
    railway_runtime_mark_clean(RAILWAY_DATA_USERS);
    return SUCCESS;
}

void loadUserData(void)
{
    FILE *fp;

    fp = fopen(
        USER_FILE,
        "r");

    if(fp == NULL)
    {
        return;
    }

    char line[256];

    while(fgets(
          line,
          sizeof(line),
          fp))
    {
        line[strcspn(line, "\r\n")] = '\0';

        if(strcmp(
           line,
           "USER_START") == 0)
        {
            User *newUser =
            (User *)malloc(
            sizeof(User));

            if(newUser == NULL)
            {
                fclose(fp);
                return;
            }

            memset(
            newUser,
            0,
            sizeof(User));

            fgets(
            line,
            sizeof(line),
            fp);

            newUser->passengerId =
                atoi(line);

            if (!fgets(line, sizeof line, fp)) { free(newUser); break; }
            line[strcspn(line, "\r\n")] = '\0';
            if (strlen(line) < sizeof newUser->username) strcpy(newUser->username, line);
            else newUser->username[0] = '\0';

            if (!fgets(line, sizeof line, fp)) { free(newUser); break; }
            line[strcspn(line, "\r\n")] = '\0';
            if (strlen(line) < sizeof newUser->password) strcpy(newUser->password, line);
            else newUser->password[0] = '\0';

            if (!fgets(line, sizeof(line), fp)) { free(newUser); break; }
            line[strcspn(line, "\r\n")] = '\0';
            if (strcmp(line, "USER_END") != 0) {
                printf("\nWARNING: Invalid user record skipped.\n");
                free(newUser);
                continue;
            }

            if (newUser->passengerId <= 0 || newUser->username[0] == '\0' || newUser->password[0] == '\0' ||
                userExists(newUser->passengerId) || usernameExists(newUser->username)) {
                printf("\nWARNING: Invalid or duplicate user record skipped.\n");
                free(newUser);
                continue;
            }

            newUser->prev = NULL;
            newUser->next =
                userHead;

            if(userHead != NULL)
            {
                userHead->prev =
                    newUser;
            }

            userHead =
                newUser;
        }
    }

    fclose(fp);
}

static int userExists(
    int passengerId)
{
    User *current =
        userHead;

    while(current != NULL)
    {
        if(current->passengerId ==
           passengerId)
        {
            return SUCCESS;
        }

        current =
            current->next;
    }

    return FAILURE;
}
