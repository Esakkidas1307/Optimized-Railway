#include "adminmenu.h"
#include <setjmp.h>

#include "utils.h"
#include "date_time.h"

#include "train.h"
#include "passenger.h"
#include "trainschedule.h"
#include "availability.h"
#include "reserved_booking.h"

void adminMenu(void)
{
    
    volatile int invalidChoices = 0;
    jmp_buf menu_env;

    /* Each invalid nested input jumps back here, abandoning the current operation. */
    railway_set_navigation_target(&menu_env);
    (void)setjmp(menu_env);

    while(1)
    {
        int choice;
        printf(
        "\n===== ADMIN MENU =====");

        printf(
        "\n1. Add Train");

        printf(
        "\n2. View Trains");

        printf(
        "\n3. Search Train");

        printf(
        "\n4. Update Train");

        printf(
        "\n5. Delete Train");

        printf(
        "\n6. View Passengers");

        printf(
        "\n7. Search Passenger");

        printf(
        "\n8. Delete Passenger");

        printf(
        "\n9. Add Train Schedule");

        printf(
        "\n10. View Train Schedules");

        printf(
        "\n11. Search Train Schedule");

        printf(
        "\n12. Update Train Schedule");

        printf(
        "\n13. Delete Train Schedule");

        printf("\n14. View Availability");

        printf("\n15. Logout");
        choice =getIntegerInput("\nEnter choice (1-15; 15 to logout): ");

        if (choice >= 1 && choice <= 15) invalidChoices = 0;
        if (choice < 1 || choice > 15) {
            invalidChoices++;
            printf("\nInvalid Choice (%d/3).\n", invalidChoices);
            if (invalidChoices >= 3) {
                railway_log_event("adminlog.txt", "admin", "LOGOUT");
                railway_set_navigation_target(NULL);
                printf("Three invalid admin-menu choices. Returning to Main Menu.\n");
                return;
            }
            continue;
        }
        switch(choice)
        {
            case 1:
            {
                addTrain();
                break;
            }

            case 2:
            {
                viewTrainDetails();
                break;
            }

            case 3:
            {
                searchTrain();
                break;
            }

            case 4:
            {
                updateTrain();
                break;
            }

            case 5:
            {
                deleteTrain();
                break;
            }

            case 6:
            {
                viewPassengers();
                break;
            }

            case 7:
            {
                searchPassenger();
                break;
            }

            case 8:
            {
                deletePassenger();
                break;
            }

            case 9:
            {
                addTrainSchedule();
                break;
            }

            case 10:
            {
                viewTrainSchedules();
                break;
            }

            case 11:
            {
                searchTrainSchedule();
                break;
            }

            case 12:
            {
                updateTrainSchedule();
                break;
            }

            case 13:
            {
                deleteTrainSchedule();
                break;
            }

            case 14:
            {
                checkAvailability();
                break;
            }
            
            case 15:
            {
                railway_log_event("adminlog.txt", "admin", "LOGOUT");
                railway_set_navigation_target(NULL);
                return;
            }

            default: break;
        }
    }
}
