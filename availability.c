#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "availability.h"
#include "train.h"
#include "trainschedule.h"
#include "utils.h"
#include "optimization_runtime.h"

#define AVAILABILITY_FILE "running.txt"
#define TEMP_AVAILABILITY_FILE "running.txt.tmp"

Availability *availabilityHead = NULL;

/* Prevent inventory from being loaded repeatedly during one execution. */
static int availabilityLoaded = 0;


static int supportedInventoryClass(const char *name)
{
    static const char *allowed[] = {"SL", "1A", "2A", "3A", "3E", "2S", "EC", "CC"};
    for (size_t i = 0; i < sizeof allowed / sizeof allowed[0]; ++i)
        if (strcmp(name, allowed[i]) == 0) return 1;
    return 0;
}

/* ---------------------------------------------------------
   FIND SCHEDULE
   --------------------------------------------------------- */
static const TrainSchedule *findScheduleForDate(int trainNumber, const char *journeyDate)
{
    return findTrainScheduleByDate(trainNumber, journeyDate);
}


/* ---------------------------------------------------------
   INITIALIZE AVAILABILITY MODULE

   IMPORTANT:
   - Never copy train capacities here.
   - Never reset existing seat counts here.
   - Only load running.txt.
   --------------------------------------------------------- */
void initializeAvailabilityModule(void)
{
    if (availabilityLoaded)
    {
        return;
    }

    FILE *fp = fopen(AVAILABILITY_FILE, "a");

    if (fp == NULL)
    {
        printf("\nERROR: Cannot access %s\n", AVAILABILITY_FILE);
        return;
    }

    fclose(fp);

    /* Load existing inventory only. */
    loadAvailabilityData();

    availabilityLoaded = 1;

    printf("\nRunning inventory loaded successfully.\n");
}


/* ---------------------------------------------------------
   CHECK AVAILABILITY

   This function ONLY READS inventory.
   It must never create or reset inventory.
   --------------------------------------------------------- */
void checkAvailability(void)
{
    int trainNumber;
    int found = FAILURE;
    char journeyDate[11];

    while (1) {
        trainNumber = getIntegerInput("\nTrain number (0 to go back): ");
        if (trainNumber == 0) return;
        if (findTrain(trainNumber) != NULL) break;
        printf("Train not found. Try again.\n");
    }
    while (1) {
        getStringInput("Journey date DD:MM:YYYY: ", journeyDate, sizeof(journeyDate));
        if (isValidJourneyDate(journeyDate)) break;
        printf("Invalid calendar date. Use DD:MM:YYYY, for example 24:10:2026.\n");
    }

    const TrainSchedule *schedule =
        findScheduleForDate(trainNumber, journeyDate);

    if (schedule == NULL)
    {
        printf("\nNo schedule found for train %d on %s.\n",
               trainNumber, journeyDate);
        return;
    }

    if (schedule->status == TRAIN_CANCELLED)
    {
        printf("\nThis train is cancelled for %s.\n", journeyDate);
        return;
    }

    Availability *current = availabilityHead;

    while (current != NULL)
    {
        if (current->trainNumber == trainNumber &&
            strcmp(current->journeyDate, journeyDate) == 0)
        {
            found = SUCCESS;

            printf("\n==============================\n");
            printf("Train Number : %d\n", current->trainNumber);
            printf("Journey Date : %s\n", current->journeyDate);
            printf("Class        : %s\n", current->className);

            if (current->availableSeats > 0)
            {
                printf("Available: %d\n", current->availableSeats);
            }
            else if (current->availableRAC > 0)
            {
                printf("RAC: %d\n", current->availableRAC);
            }
            else
            {
                const Train *train = findTrain(current->trainNumber);
                int wlLimit = 0;

                if (train != NULL)
                {
                    for (int i = 0; i < train->classCount; i++)
                    {
                        if (strcmp(train->coaches[i].className,
                                   current->className) == 0)
                        {
                            wlLimit = train->coaches[i].wlCapacity;
                            break;
                        }
                    }
                }

                if (current->availableWL < wlLimit)
                {
                    printf("WL: %d\n", current->availableWL + 1);
                }
                else
                {
                    printf("No availability\n");
                }
            }

            printf("==============================\n");
        }

        current = current->next;
    }

    if (!found)
    {
        printf("\nSchedule exists, but no inventory record exists "
               "in %s for this train and date.\n",
               AVAILABILITY_FILE);

        printf("Add or initialize the inventory through the "
               "Add Train Schedule operation.\n");
    }
}


/* ---------------------------------------------------------
   CREATE AVAILABILITY RECORD

   Call this from addTrainSchedule() ONLY.

   If a record already exists, do not reset it.
   If no record exists, initialize it from trains.txt data.
   --------------------------------------------------------- */
void createAvailabilityRecord(Train *train, const char *journeyDate)
{
    if (train == NULL || journeyDate == NULL ||
        journeyDate[0] == '\0')
    {
        return;
    }

    int created = 0;

    for (int i = 0; i < train->classCount; i++)
    {
        const char *className = train->coaches[i].className;

        /* Existing inventory must never be overwritten. */
        if (findAvailability(train->trainNumber,
                             journeyDate,
                             className) != NULL)
        {
            continue;
        }

        Availability *newNode =
            (Availability *)malloc(sizeof(Availability));

        if (newNode == NULL)
        {
            printf("\nMemory allocation failed while creating "
                   "availability.\n");
            return;
        }

        memset(newNode, 0, sizeof(Availability));

        newNode->trainNumber = train->trainNumber;

        snprintf(newNode->journeyDate,
                 sizeof(newNode->journeyDate),
                 "%s", journeyDate);

        snprintf(newNode->className,
                 sizeof(newNode->className),
                 "%s", className);

        newNode->availableSeats = train->coaches[i].totalSeats;
        newNode->availableRAC = train->coaches[i].racCapacity;

        /* WL stores the last allocated WL number. */
        newNode->availableWL = 0;

        newNode->prev = NULL;
        newNode->next = availabilityHead;

        if (availabilityHead != NULL)
        {
            availabilityHead->prev = newNode;
        }

        availabilityHead = newNode;
        created = 1;
    }

    /*
       Save only if new records were created.
       Existing records retain their current seat counts.
    */
    if (created)
    {
        if (!saveAvailabilityData())
        {
            printf("\nERROR: Initial inventory could not be saved.\n");
        }
        else
        {
            printf("\nInitial inventory created for train %d, date %s.\n",
                   train->trainNumber, journeyDate);
        }
    }
}


/* ---------------------------------------------------------
   FIND AVAILABILITY RECORD
   --------------------------------------------------------- */
Availability *findAvailability(int trainNumber,
                               const char *journeyDate,
                               const char *className)
{
    if (journeyDate == NULL || className == NULL)
    {
        return NULL;
    }

    Availability *current = availabilityHead;

    while (current != NULL)
    {
        if (current->trainNumber == trainNumber &&
            strcmp(current->journeyDate, journeyDate) == 0 &&
            strcmp(current->className, className) == 0)
        {
            return current;
        }

        current = current->next;
    }

    return NULL;
}


/* ---------------------------------------------------------
   SAVE AVAILABILITY DATA

   Writes the complete current inventory to a temporary file,
   then replaces running.txt only after a successful write.
   --------------------------------------------------------- */
int saveAvailabilityData(void)
{
    FILE *fp;
    railway_runtime_mark_dirty(RAILWAY_DATA_RUNNING);
    fp = fopen(TEMP_AVAILABILITY_FILE, "w");

    if (fp == NULL)
    {
        printf("\nERROR: Cannot open temporary inventory file.\n");
        return FAILURE;
    }

    Availability *current = availabilityHead;

    while (current != NULL)
    {
        if (fprintf(fp,
                    "AVAILABILITY_START\n"
                    "%d\n"
                    "%s\n"
                    "%s\n"
                    "%d\n"
                    "%d\n"
                    "%d\n"
                    "AVAILABILITY_END\n",
                    current->trainNumber,
                    current->journeyDate,
                    current->className,
                    current->availableSeats,
                    current->availableRAC,
                    current->availableWL) < 0)
        {
            fclose(fp);
            remove(TEMP_AVAILABILITY_FILE);

            printf("\nERROR: Failed writing inventory.\n");
            return FAILURE;
        }

        current = current->next;
    }

    if (fflush(fp) != 0 || ferror(fp))
    {
        fclose(fp);
        remove(TEMP_AVAILABILITY_FILE);

        printf("\nERROR: Failed flushing inventory file.\n");
        return FAILURE;
    }

    if (fclose(fp) != 0)
    {
        remove(TEMP_AVAILABILITY_FILE);

        printf("\nERROR: Failed closing inventory file.\n");
        return FAILURE;
    }

    /*
       On OnlineGDB/Linux, rename replaces running.txt with
       the saved inventory.
    */
    if (rename(TEMP_AVAILABILITY_FILE, AVAILABILITY_FILE) != 0)
    {
        printf("\nERROR: Could not update %s.\n",
               AVAILABILITY_FILE);

        remove(TEMP_AVAILABILITY_FILE);
        return FAILURE;
    }

    railway_runtime_mark_clean(RAILWAY_DATA_RUNNING);
    return SUCCESS;
}


/* ---------------------------------------------------------
   LOAD AVAILABILITY DATA

   Loads existing records only.
   Does not initialize capacities from trains.txt.
   --------------------------------------------------------- */
void loadAvailabilityData(void)
{
    FILE *fp = fopen(AVAILABILITY_FILE, "r");
    char line[128];

    if (fp == NULL)
        return;

    /* Prevent duplicate in-memory records if called repeatedly. */
    while (availabilityHead != NULL)
    {
        Availability *next = availabilityHead->next;
        free(availabilityHead);
        availabilityHead = next;
    }

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strcspn(line, "\r\n")] = '\0';
        if (strcmp(line, "AVAILABILITY_START") != 0)
            continue;

        Availability record;
        memset(&record, 0, sizeof(record));
        int valid = 1;
        char *endptr;
        long value;

        /* Train number */
        if (fgets(line, sizeof(line), fp) == NULL) break;
        line[strcspn(line, "\r\n")] = '\0';
        value = strtol(line, &endptr, 10);
        if (*line == '\0' || *endptr != '\0' || value <= 0)
            valid = 0;
        record.trainNumber = (int)value;

        /* Journey date */
        if (fgets(line, sizeof(line), fp) == NULL) break;
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0' || strlen(line) >= sizeof(record.journeyDate))
            valid = 0;
        else
            strcpy(record.journeyDate, line);

        /* Class */
        if (fgets(line, sizeof(line), fp) == NULL) break;
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0' || strlen(line) >= sizeof(record.className))
            valid = 0;
        else
            strcpy(record.className, line);

        /* Available seats */
        if (fgets(line, sizeof(line), fp) == NULL) break;
        line[strcspn(line, "\r\n")] = '\0';
        value = strtol(line, &endptr, 10);
        if (*line == '\0' || *endptr != '\0' || value < 0) valid = 0;
        record.availableSeats = (int)value;

        /* Available RAC */
        if (fgets(line, sizeof(line), fp) == NULL) break;
        line[strcspn(line, "\r\n")] = '\0';
        value = strtol(line, &endptr, 10);
        if (*line == '\0' || *endptr != '\0' || value < 0) valid = 0;
        record.availableRAC = (int)value;

        /* Last allocated WL number */
        if (fgets(line, sizeof(line), fp) == NULL) break;
        line[strcspn(line, "\r\n")] = '\0';
        value = strtol(line, &endptr, 10);
        if (*line == '\0' || *endptr != '\0' || value < 0) valid = 0;
        record.availableWL = (int)value;

        /* End marker */
        if (fgets(line, sizeof(line), fp) == NULL) break;
        long linePos = ftell(fp) - (long)strlen(line);
        line[strcspn(line, "\r\n")] = '\0';

        if (strcmp(line, "AVAILABILITY_END") != 0)
        {
            printf("\nWARNING: Invalid inventory record skipped.\n");

            if (strcmp(line, "AVAILABILITY_START") == 0)
            {
                fseek(fp, linePos, SEEK_SET);
            }
            else
            {
                while (fgets(line, sizeof(line), fp) != NULL)
                {
                    long pos = ftell(fp) - (long)strlen(line);
                    line[strcspn(line, "\r\n")] = '\0';
                    if (strcmp(line, "AVAILABILITY_START") == 0)
                    {
                        fseek(fp, pos, SEEK_SET);
                        break;
                    }
                }
            }
            continue;
        }

        if (!supportedInventoryClass(record.className) || !findScheduleForDate(record.trainNumber, record.journeyDate)) valid = 0;
        const Train *inventoryTrain = findTrain(record.trainNumber);
        int classFound = 0;
        if (inventoryTrain) for (int ci = 0; ci < inventoryTrain->classCount; ++ci)
            if (strcmp(inventoryTrain->coaches[ci].className, record.className) == 0) {
                classFound = 1;
                if (record.availableSeats > inventoryTrain->coaches[ci].totalSeats || record.availableRAC > inventoryTrain->coaches[ci].racCapacity || record.availableWL > inventoryTrain->coaches[ci].wlCapacity) valid = 0;
            }
        if (!classFound) valid = 0;
        if (!valid)
        {
            printf("\nWARNING: Invalid inventory record skipped.\n");
            continue;
        }

        if (findAvailability(record.trainNumber,
                             record.journeyDate,
                             record.className) != NULL)
        {
            /* Never let duplicate lines overwrite already-loaded inventory. */
            continue;
        }

        Availability *node = malloc(sizeof(*node));
        if (node == NULL)
        {
            printf("\nMemory allocation failed while loading inventory.\n");
            fclose(fp);
            return;
        }

        *node = record;
        node->prev = NULL;
        node->next = availabilityHead;
        if (availabilityHead != NULL)
            availabilityHead->prev = node;
        availabilityHead = node;
    }

    fclose(fp);
}
