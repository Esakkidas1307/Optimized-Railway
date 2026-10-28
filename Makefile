CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wconversion -Wsign-conversion
LDLIBS ?= -pthread -lm

SOURCES = main.c admin_auth.c adminmenu.c availability.c cancellation.c \
          date_time.c passenger.c pnr_status.c reallocation.c refund.c \
          reserved_booking.c train.c train_search.c trainschedule.c \
          unreserved_booking.c user.c usermenu.c utils.c \
          optimization_runtime.c optimization_support.c

.PHONY: all clean
all: railway_app

railway_app: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LDLIBS)

clean:
	rm -f railway_app
