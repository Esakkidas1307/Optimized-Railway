#include "usermenu.h"
#include "passenger.h"
#include "utils.h"
#include "reserved_booking.h"
#include "unreserved_booking.h"
#include "availability.h"
#include "date_time.h"
#include "user.h"
#include <setjmp.h>

void userMenu(int passengerId)
{
    
    volatile int invalidChoices = 0;
    jmp_buf menu_env;
    railway_set_navigation_target(&menu_env);
    (void)setjmp(menu_env);

    while(1)
    {
        int choice;
        printf("\n===== USER MENU =====");

        printf("\n1. Create Profile");

        printf("\n2. View My Profile");

        printf("\n3. Update My Profile");

        printf("\n4. Check Availability");
        printf("\n5. Reserved Booking");
        printf("\n6. Unreserved Booking");
        printf("\n7. View My Bookings");
        printf("\n8. Cancel Ticket");
        printf("\n0. Logout");

        choice =
        getIntegerInput(
        "\nEnter choice (0-8; 0 to logout): ");

        if (choice >= 0 && choice <= 8) invalidChoices = 0;
        if (choice < 0 || choice > 8) {
            invalidChoices++;
            printf("\nInvalid Choice (%d/3).\n", invalidChoices);
            if (invalidChoices >= 3) {
                User *u = userHead;
                while (u && u->passengerId != passengerId) u = u->next;
                railway_log_event("userlog.txt", u ? u->username : "unknown", "LOGOUT");
                railway_set_navigation_target(NULL);
                printf("Three invalid user-menu choices. Returning to Main Menu.\n");
                return;
            }
            continue;
        }
        switch(choice)
        {
            case 1:
                createPassengerProfile(passengerId);
                break;

            case 2:
                viewMyProfile(passengerId);
                break;

            case 3:
                updateMyProfile(passengerId);
                break;

            case 4:
                checkAvailability();
                break;

            case 5:
                bookTicket(passengerId);
                break;

            case 6:
                bookUnreservedTicket(passengerId);
                break;

            case 7:
                viewMyBookings(passengerId);
                break;

            case 8:
                cancelTicket(passengerId);
                break;

            case 0:
            {
                User *u = userHead;
                while (u && u->passengerId != passengerId) u = u->next;
                railway_log_event("userlog.txt", u ? u->username : "unknown", "LOGOUT");
                railway_set_navigation_target(NULL);
                return;
            }

            default: break;
        }
    }
}
