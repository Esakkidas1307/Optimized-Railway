#include "passenger.h"
#include "utils.h"
#include "optimization_support.h"
#include "optimization_runtime.h"

#define PASSENGER_FILE "passengers.txt"
#define PASSENGER_TEMP_FILE "passengers.txt.tmp"

Passenger *passengerHead = NULL;
static RailwayPointerIndex passengerIndex = {NULL, 0U, 0U};
static int passengerIndexReady;

static int comparePassengerPointers(const void *left, const void *right)
{
    const Passenger *a = (const Passenger *)left;
    const Passenger *b = (const Passenger *)right;
    if (a->passengerId < b->passengerId) return -1;
    if (a->passengerId > b->passengerId) return 1;
    return 0;
}

static int rebuildPassengerIndex(void)
{
    Passenger *node;
    Passenger **items;
    size_t count = 0U;
    size_t i = 0U;
    for (node = passengerHead; node != NULL; node = node->next) ++count;
    items = count == 0U ? NULL : (Passenger **)malloc(count * sizeof(*items));
    if (count > 0U && items == NULL) return FAILURE;
    for (node = passengerHead; node != NULL; node = node->next) items[i++] = node;
    if (railway_pointer_index_build(&passengerIndex, (void *const *)items, count, comparePassengerPointers) != 0) {
        free(items);
        return FAILURE;
    }
    free(items);
    passengerIndexReady = 1;
    return SUCCESS;
}

static int passengerIdExists(int passengerId);

static int validateName(const char *name);

static int validatePhoneNumber(const char *phoneNumber);

static void getValidName(const char *message,char *buffer);

static void getValidPhoneNumber(const char *message,char *buffer);

void viewMyProfile(int passengerId)
{
    Passenger *current;
    if (passengerIndexReady != 0) {
        Passenger key;
        memset(&key, 0, sizeof(key));
        key.passengerId = passengerId;
        current = (Passenger *)railway_pointer_index_find(&passengerIndex, &key, comparePassengerPointers);
    } else {
        current = passengerHead;
        while (current != NULL && current->passengerId != passengerId) current = current->next;
    }
    if (current == NULL) {
        printf("\nProfile Not Found\n");
        return;
    }
    printf("\nPassenger ID : %d", current->passengerId);
    printf("\nName : %s", current->passengerName);
    printf("\nAge : %d", current->age);
    printf("\nGender : %s", current->gender);
    printf("\nPhone : %s\n", current->phoneNumber);
}
    
void initializePassengerModule(void)
{
    FILE *fp;

    fp = fopen(PASSENGER_FILE, "a");

    if(fp == NULL)
    {
        printf(
        "\nUnable To Create Passenger File\n");

        return;
    }

    fclose(fp);

    loadPassengerData();
}

static int passengerIdExists(int passengerId)
{
    if (passengerIndexReady != 0) {
        Passenger key;
        key.passengerId = passengerId;
        return railway_pointer_index_find(&passengerIndex, &key, comparePassengerPointers) != NULL ? SUCCESS : FAILURE;
    }
    Passenger *current = passengerHead;
    while (current != NULL) {
        if (current->passengerId == passengerId) return SUCCESS;
        current = current->next;
    }
    return FAILURE;
}

static int validateName(
    const char *name)
{
    int i;

    if(strlen(name) == 0)
    {
        return FAILURE;
    }

    for(i = 0;
        name[i] != '\0';
        i++)
    {
        if(
           (name[i] >= 'A' &&
            name[i] <= 'Z')
           ||
           (name[i] >= 'a' &&
            name[i] <= 'z')
           ||
           (name[i] == ' ')
           || name[i] == '\'' || name[i] == '-'
          )
        {
            continue;
        }

        return FAILURE;
    }

    return SUCCESS;
}

static int validatePhoneNumber(
    const char *phoneNumber)
{
    int i;

    if(strlen(phoneNumber) != 10)
    {
        return FAILURE;
    }

    if (phoneNumber[0] < '6' || phoneNumber[0] > '9') return FAILURE;

    for(i = 0;
        phoneNumber[i] != '\0';
        i++)
    {
        if(phoneNumber[i] < '0' ||
           phoneNumber[i] > '9')
        {
            return FAILURE;
        }
    }

    return SUCCESS;
}

static void getValidName(
    const char *message,
    char *buffer)
{
    while(1)
    {
        getStringInput(
            message,
            buffer,
            MAX_NAME_LEN);

        if(validateName(buffer))
        {
            break;
        }

        printf(
        "Invalid Name.\n");
    }
}

static void getValidPhoneNumber(
    const char *message,
    char *buffer)
{
    while(1)
    {
        getStringInput(
            message,
            buffer,
            MAX_PHONE_LEN);

        if(validatePhoneNumber(
            buffer))
        {
            break;
        }

        printf(
        "Invalid Phone Number.\n");
    }
}

void createPassengerProfile(
    int passengerId)
{
    Passenger *newPassenger;

    if(passengerIdExists(
       passengerId))
    {
        printf(
        "\nProfile Already Exists\n");

        return;
    }

    newPassenger =
    (Passenger *)malloc(
    sizeof(Passenger));

    if(newPassenger == NULL)
    {
        printf(
        "\nMemory Allocation Failed\n");

        return;
    }

newPassenger->passengerId =passengerId;


getValidName("Passenger name: ",newPassenger->passengerName);

    while(1)
    {
        newPassenger->age =
            getIntegerInput(
            "Age in years (1-120, e.g., 22): ");

        if(newPassenger->age >= 1 &&
           newPassenger->age <= 120)
        {
            break;
        }

        printf(
        "Invalid Age.\n");
    }

    while(1)
    {
        getStringInput(
            "Gender (M/F/O): ",
            newPassenger->gender,
            MAX_GENDER_LEN);

        if(strcmp(
            newPassenger->gender,
            "M") == 0)
        {
            strcpy(
            newPassenger->gender,
            "Male");

            break;
        }

        if(strcmp(
            newPassenger->gender,
            "F") == 0)
        {
            strcpy(
            newPassenger->gender,
            "Female");

            break;
        }

        if(strcmp(
            newPassenger->gender,
            "O") == 0)
        {
            strcpy(
            newPassenger->gender,
            "Other");

            break;
        }

        printf(
        "Invalid Gender.\n");
    }

    getValidPhoneNumber(
        "Indian mobile number, 10 digits (e.g., 9876543210): ",
        newPassenger->phoneNumber);

    newPassenger->prev = NULL;

    newPassenger->next =
        passengerHead;

    if(passengerHead != NULL)
    {
        passengerHead->prev =
            newPassenger;
    }

    passengerHead =
        newPassenger;

    savePassengerData();

    printf(
    "\nPassenger Created Successfully\n");
}

void viewPassengers(void)
{
    Passenger *current =
        passengerHead;

    if(current == NULL)
    {
        printf(
        "\nNo Passenger Available\n");

        return;
    }

    while(current != NULL)
    {
        printf(
        "\n================================");

        printf(
        "\nPassenger ID : %d",
        current->passengerId);

        printf(
        "\nPassenger Name : %s",
        current->passengerName);

        printf(
        "\nAge : %d",
        current->age);

        printf(
        "\nGender : %s",
        current->gender);

        printf(
        "\nPhone Number : %s",
        current->phoneNumber);

        printf(
        "\n================================\n");

        current =
            current->next;
    }
}

int savePassengerData(void)
{
    FILE *fp;
    size_t position;
    railway_runtime_mark_dirty(RAILWAY_DATA_PASSENGERS);
    if (rebuildPassengerIndex() != SUCCESS) return FAILURE;
    fp = fopen(PASSENGER_TEMP_FILE, "w");
    if (fp == NULL) return FAILURE;
    for (position = 0U; position < passengerIndex.count; ++position) {
        Passenger *current = (Passenger *)passengerIndex.items[position];
        if (fprintf(fp, "PASSENGER_START\n%d\n%s\n%d\n%s\n%s\nPASSENGER_END\n",
                    current->passengerId, current->passengerName, current->age,
                    current->gender, current->phoneNumber) < 0) {
            (void)fclose(fp);
            (void)remove(PASSENGER_TEMP_FILE);
            return FAILURE;
        }
    }
    if (fflush(fp) != 0 || ferror(fp)) {
        (void)fclose(fp);
        (void)remove(PASSENGER_TEMP_FILE);
        return FAILURE;
    }
    if (fclose(fp) != 0) {
        (void)remove(PASSENGER_TEMP_FILE);
        return FAILURE;
    }
    if (rename(PASSENGER_TEMP_FILE, PASSENGER_FILE) != 0) {
        (void)remove(PASSENGER_TEMP_FILE);
        return FAILURE;
    }
    railway_runtime_mark_clean(RAILWAY_DATA_PASSENGERS);
    return SUCCESS;
}

void loadPassengerData(void)
{
    FILE *fp;

    fp = fopen(
            PASSENGER_FILE,
            "r");

    if(fp == NULL)
    {
        return;
    }

    char line[100];

    while(fgets(
          line,
          sizeof(line),
          fp))
    {
        line[strcspn(line, "\r\n")] = '\0';

        if(strcmp(
           line,
           "PASSENGER_START") == 0)
        {
            Passenger *newPassenger =
            (Passenger *)malloc(
            sizeof(Passenger));

            if(newPassenger == NULL)
            {
                fclose(fp);
                return;
            }

            memset(
            newPassenger,
            0,
            sizeof(Passenger));

            fgets(
            line,
            sizeof(line),
            fp);

            newPassenger->passengerId =
                atoi(line);

            fgets(
            newPassenger->passengerName,
            MAX_NAME_LEN,
            fp);

            newPassenger->passengerName[strcspn(newPassenger->passengerName, "\r\n")] = '\0';

            fgets(
            line,
            sizeof(line),
            fp);

            newPassenger->age =
                atoi(line);

            fgets(
            newPassenger->gender,
            MAX_GENDER_LEN,
            fp);

            newPassenger->gender[strcspn(newPassenger->gender, "\r\n")] = '\0';

            fgets(
            newPassenger->phoneNumber,
            MAX_PHONE_LEN,
            fp);

            newPassenger->phoneNumber[strcspn(newPassenger->phoneNumber, "\r\n")] = '\0';

            if (!fgets(line, sizeof(line), fp)) { free(newPassenger); break; }
            line[strcspn(line, "\r\n")] = '\0';
            if (strcmp(line, "PASSENGER_END") != 0) {
                printf("\nWARNING: Invalid passenger record skipped.\n");
                free(newPassenger);
                continue;
            }

            int duplicateId = 0;
            for (Passenger *scan = passengerHead; scan != NULL; scan = scan->next)
                if (scan->passengerId == newPassenger->passengerId) duplicateId = 1;
            int validGender = strcmp(newPassenger->gender, "Male") == 0 || strcmp(newPassenger->gender, "Female") == 0 || strcmp(newPassenger->gender, "Other") == 0;
            if (newPassenger->passengerId <= 0 || !validateName(newPassenger->passengerName) ||
                newPassenger->age < 1 || newPassenger->age > 120 || !validGender ||
                !validatePhoneNumber(newPassenger->phoneNumber) || duplicateId) {
                printf("\nWARNING: Invalid passenger record skipped.\n");
                free(newPassenger);
                continue;
            }

            newPassenger->prev = NULL;

            newPassenger->next =
                passengerHead;

            if(passengerHead != NULL)
            {
                passengerHead->prev =
                    newPassenger;
            }

            passengerHead =
                newPassenger;
        }
    }

    fclose(fp);
    (void)rebuildPassengerIndex();
}

void searchPassenger(void)
{
    int choice;

    Passenger *current = passengerHead;

    choice =
        getIntegerInput(
        "\n1. Search By Passenger ID"
        "\n2. Search By Phone Number"
        "\nEnter Choice : ");

    switch(choice)
    {
        case 1:
        {
            int passengerId =
                getIntegerInput(
                "Passenger ID : ");

            if (passengerIndexReady != 0) {
                Passenger key;
                memset(&key, 0, sizeof(key));
                key.passengerId = passengerId;
                current = (Passenger *)railway_pointer_index_find(&passengerIndex, &key, comparePassengerPointers);
            } else {
                current = passengerHead;
                while (current != NULL && current->passengerId != passengerId) current = current->next;
            }
            if (current != NULL)
            {
                    printf(
                    "\nPassenger Found");

                    printf(
                    "\nPassenger ID : %d",
                    current->passengerId);

                    printf(
                    "\nPassenger Name : %s",
                    current->passengerName);

                    printf(
                    "\nAge : %d",
                    current->age);

                    printf(
                    "\nGender : %s",
                    current->gender);

                    printf(
                    "\nPhone Number : %s\n",
                    current->phoneNumber);

                    return;
            }
            break;
        }

        case 2:
        {
            char phoneNumber[MAX_PHONE_LEN];
            getValidPhoneNumber(
                "Indian mobile number, 10 digits (e.g., 9876543210): ",
                phoneNumber);

            while(current != NULL)
            {
                if(strcmp(
                   current->phoneNumber,
                   phoneNumber) == 0)
                {
                    printf(
                    "\nPassenger Found");

                    printf(
                    "\nPassenger ID : %d",
                    current->passengerId);

                    printf(
                    "\nPassenger Name : %s",
                    current->passengerName);

                    printf(
                    "\nAge : %d",
                    current->age);

                    printf(
                    "\nGender : %s",
                    current->gender);

                    printf(
                    "\nPhone Number : %s\n",
                    current->phoneNumber);

                    return;
                }

                current =
                    current->next;
            }

            break;
        }

        default:
        {
            printf(
            "\nInvalid Choice\n");

            return;
        }
    }

    printf(
    "\nPassenger Not Found\n");
}

void updateMyProfile(int passengerId)
{
    int choice;

    Passenger *current =passengerHead;

    while(current != NULL)
    {
        if(current->passengerId ==passengerId)
        {
            printf(
            "\nPassenger Found");

            printf(
            "\n1. Update Name");

            printf(
            "\n2. Update Age");

            printf(
            "\n3. Update Phone Number");

            choice =
                getIntegerInput(
                "\nEnter Choice : ");

            switch(choice)
            {
                case 1:
                {
                    getValidName("New Name : ",current->passengerName);

                    break;
                }

                case 2:
                {
                    while(1)
                    {
                        current->age =
                            getIntegerInput(
                            "New Age : ");

                        if(current->age >= 1 &&
                           current->age <= 120)
                        {
                            break;
                        }

                        printf(
                        "Invalid Age.\n");
                    }

                    break;
                }

                case 3:
                {
                    getValidPhoneNumber(
                        "New Phone Number : ",
                        current->phoneNumber);

                    break;
                }

                default:
                {
                    printf(
                    "\nInvalid Choice\n");

                    return;
                }
            }

            savePassengerData();

            printf(
            "\nPassenger Updated Successfully\n");

            return;
        }

        current =
            current->next;
    }

    printf(
    "\nPassenger Not Found\n");
}

void deletePassenger(void)
{
    int passengerId;

    Passenger *current =
        passengerHead;

    passengerId =
        getIntegerInput(
        "\nEnter Passenger ID To Delete : ");

    while(current != NULL)
    {
        if(current->passengerId ==
           passengerId)
        {
            char confirm[8];
            getStringInput("Confirm passenger deletion? (Y/N): ", confirm, sizeof confirm);
            if (strcmp(confirm, "Y") != 0 && strcmp(confirm, "y") != 0) { printf("Deletion aborted.\n"); return; }
            if(current->prev == NULL)
            {
                passengerHead =
                    current->next;

                if(passengerHead != NULL)
                {
                    passengerHead->prev =
                        NULL;
                }
            }
            else
            {
                current->prev->next =
                    current->next;

                if(current->next != NULL)
                {
                    current->next->prev =
                        current->prev;
                }
            }

            free(current);

            savePassengerData();

            printf(
            "\nPassenger Deleted Successfully\n");

            return;
        }

        current =
            current->next;
    }

    printf(
    "\nPassenger Not Found\n");
}
