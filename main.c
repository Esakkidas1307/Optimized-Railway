#include "utils.h"
#include "train.h"
#include "passenger.h"
#include "user.h"
#include "trainschedule.h"
#include "usermenu.h"
#include "adminmenu.h"
#include "availability.h"
#include "reserved_booking.h"
#include "pnr_status.h"
#include "admin_auth.h"
#include <setjmp.h>
#include "optimization_runtime.h"

int loggedInPassengerId = FAILURE;

static int save_trains_callback(void) { return saveTrainData(); }
static int save_schedules_callback(void) { return saveTrainScheduleData(); }
static int save_passengers_callback(void) { return savePassengerData(); }
static int save_running_callback(void) { return saveAvailabilityData(); }
static int save_users_callback(void) { return saveUserData(); }
static int save_bookings_callback(void) { return saveBookingData(); }

static void register_persistence_callbacks(void)
{
    (void)railway_runtime_register_saver(RAILWAY_DATA_TRAINS, save_trains_callback);
    (void)railway_runtime_register_saver(RAILWAY_DATA_SCHEDULES, save_schedules_callback);
    (void)railway_runtime_register_saver(RAILWAY_DATA_PASSENGERS, save_passengers_callback);
    (void)railway_runtime_register_saver(RAILWAY_DATA_RUNNING, save_running_callback);
    (void)railway_runtime_register_saver(RAILWAY_DATA_USERS, save_users_callback);
    (void)railway_runtime_register_saver(RAILWAY_DATA_BOOKINGS, save_bookings_callback);
}

int main(void)
{
    
    volatile int invalidChoices = 0;
    jmp_buf main_env;

    initializeSystem();

    initializeTrainModule();

    initializeTrainScheduleModule();
    
    initializeAvailabilityModule();
    
    initializeBookingModule();

    initializePassengerModule();

    initializeUserModule();
    register_persistence_callbacks();

    (void)setjmp(main_env);
    if (railway_runtime_timeout_requested()) {
        cleanupSystem();
        return 0;
    }
    railway_set_navigation_target(&main_env);

    while(1)
    {
        int choice;
        railway_set_navigation_target(&main_env);
        printf(
        "\n========== MAIN MENU ==========");

        printf(
        "\n1. Register User");

        printf(
        "\n2. Login User");

        printf(
        "\n3. Admin Menu");

        printf(
        "\n4. PNR Status Verification");

        printf(
        "\n0. Exit");

        choice =
        getIntegerInput(
        "\nEnter Choice (0-4): ");

        if (choice >= 0 && choice <= 4) invalidChoices = 0;
        if (choice < 0 || choice > 4) {
            invalidChoices++;
            printf("\nInvalid Choice (%d/3).\n", invalidChoices);
            if (invalidChoices >= 3) {
                printf("Three invalid main-menu choices. Staying in Main Menu.\n");
                invalidChoices = 0;
            }
            continue;
        }
        switch(choice)
        {
            case 1:
            {
                loggedInPassengerId =
                    registerUser();

                if(loggedInPassengerId
                   != FAILURE)
                {
                    userMenu(
                    loggedInPassengerId);
                }

                break;
            }

            case 2:
            {
                loggedInPassengerId =
                    loginUser();

                if(loggedInPassengerId
                   != FAILURE)
                {
                    userMenu(
                    loggedInPassengerId);
                }

                break;
            }

            case 3:
            {
                if (adminLogin() == SUCCESS) adminMenu();
                break;
            }

            case 4:
            {
                verifyPNRStatus();
                break;
            }

            case 0:
            {
                cleanupSystem();
                return 0;
            }

            default: break;
        }
    }

    return 0;
}
