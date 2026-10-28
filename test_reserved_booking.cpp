#include <gtest/gtest.h>

#include <cstdio>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <unistd.h>

#include "reserved_booking.h"
#include "passenger.h"
#include "train.h"
#include "trainschedule.h"
#include "availability.h"
#include "utils.h"

static void addBookingTestData(
    int seats,
    int rac,
    int wl,
    int wlCapacity)
{
    auto *p = static_cast<Passenger *>(
    std::calloc(1, sizeof(Passenger)));
    if (p == nullptr) {
    return;
}
    p->passengerId = 101;
    strcpy(p->passengerName, "Test Passenger");
    p->age = 25;
    strcpy(p->gender, "Male");
    passengerHead = p;

    auto *t = static_cast<Train *>(
    std::calloc(1, sizeof(Train)));
    if (t == nullptr) {
    return;
}
    t->trainNumber = 12345;
    strcpy(t->trainName, "Test Express");
    t->totalStations = 2;
    strcpy(t->stations[0].stationName, "Chennai");
    strcpy(t->stations[1].stationName, "Bengaluru");

    strcpy(t->stations[0].departureTime, "10:00");
    strcpy(t->stations[1].arrivalTime, "16:00");

    t->stations[0].distanceFromSource = 0;
    t->stations[1].distanceFromSource = 350;
    t->classCount = 1;
    strcpy(t->coaches[0].className, "SL");
    t->coaches[0].farePerKM = 2.0f;
    t->coaches[0].wlCapacity = wlCapacity;
    trainHead = t;

    auto *s = static_cast<TrainSchedule *>(
    std::calloc(1, sizeof(TrainSchedule)));
    if (s == nullptr) {
    return;
}
    s->trainNumber = 12345;
    strcpy(s->journeyDate, "25:12:2026");
    s->status = TRAIN_RUNNING;
    scheduleHead = s;

    auto *a = static_cast<Availability *>(
    std::calloc(1, sizeof(Availability)));
    if (a == nullptr) {
    return;
}
    a->trainNumber = 12345;
    strcpy(a->journeyDate, "25:12:2026");
    strcpy(a->className, "SL");
    a->availableSeats = seats;
    a->availableRAC = rac;
    a->availableWL = wl;
    availabilityHead = a;
}

class BookingInput
{
private:
    int savedStdin;
    FILE *inputFile;

public:
    explicit BookingInput(const char *input)
        : savedStdin(-1), inputFile(nullptr)
    {
        savedStdin = dup(STDIN_FILENO);

        if (savedStdin == -1)
        {
            throw std::runtime_error("Failed to save standard input");
        }

        inputFile = tmpfile();

        if (inputFile == nullptr)
        {
            close(savedStdin);
            savedStdin = -1;
            throw std::runtime_error("Failed to create temporary input file");
        }

        const std::size_t inputLength = std::strlen(input);

        if (std::fwrite(input, 1, inputLength, inputFile) != inputLength)
        {
            std::fclose(inputFile);
            inputFile = nullptr;
            close(savedStdin);
            savedStdin = -1;
            throw std::runtime_error("Failed to write temporary input");
        }

        std::rewind(inputFile);

        if (dup2(fileno(inputFile), STDIN_FILENO) == -1)
        {
            std::fclose(inputFile);
            inputFile = nullptr;
            close(savedStdin);
            savedStdin = -1;
            throw std::runtime_error("Failed to redirect standard input");
        }

        std::clearerr(stdin);
    }

    ~BookingInput()
    {
        if (savedStdin != -1)
        {
            (void)dup2(savedStdin, STDIN_FILENO);
            (void)close(savedStdin);
        }

        if (inputFile != nullptr)
        {
            (void)std::fclose(inputFile);
        }

        std::clearerr(stdin);
    }

    BookingInput(const BookingInput &) = delete;

    BookingInput &operator=(const BookingInput &) = delete;
};

extern "C" {
#include "reserved_booking.h"
#include "utils.h"
}

class ReservedBookingTest : public ::testing::Test {
protected:
    void SetUp() override {
        clearBookings();
        std::remove("bookings.txt");
        std::remove("bookings.txt.tmp");
    }

    void TearDown() override {
        clearBookings();
        std::remove("bookings.txt");
        std::remove("bookings.txt.tmp");
    }

    void clearBookings() {
        Booking* current = bookingHead;

        while (current != nullptr) {
            Booking* next = current->next;
            std::free(current);
            current = next;
        }

        bookingHead = nullptr;
    }

    Booking* makeBooking(const char* pnr = "PNR00000001") {
        Booking* b = static_cast<Booking*>(std::calloc(1, sizeof(Booking)));
        if (!b) {
            return nullptr;
        }

        std::snprintf(b->pnrNumber, sizeof(b->pnrNumber), "%s", pnr);
        std::snprintf(b->passengerName, sizeof(b->passengerName), "Test Passenger");
        std::snprintf(b->gender, sizeof(b->gender), "Other");
        std::snprintf(b->trainName, sizeof(b->trainName), "Test Express");
        std::snprintf(b->source, sizeof(b->source), "Chennai");
        std::snprintf(b->destination, sizeof(b->destination), "Bengaluru");
        std::snprintf(b->journeyDate, sizeof(b->journeyDate), "25:12:2026");
        std::snprintf(b->className, sizeof(b->className), "SL");
        std::snprintf(b->bookingType, sizeof(b->bookingType), "RESERVED");

        b->passengerId = 1;
        b->age = 25;
        b->trainNumber = 12345;
        b->distance = 350;
        b->fare = 250.0f;
        b->ticketStatus = CNF;
        b->adults = 1;
        b->children = 0;

        return b;
    }

    void addBooking(Booking* b) {
        ASSERT_NE(b, nullptr);

        b->prev = nullptr;
        b->next = bookingHead;

        if (bookingHead != nullptr) {
            bookingHead->prev = b;
        }

        bookingHead = b;
    }

    void writeTextFile(const char* filename, const std::string& contents) {
        FILE* fp = std::fopen(filename, "w");
        ASSERT_NE(fp, nullptr);

        std::fputs(contents.c_str(), fp);
        std::fclose(fp);
    }

    std::string validRecord(const char* pnr = "PNR00000001") {
        return std::string(
            "BOOKING_START\n") + pnr +
            "\n1\nTest Passenger\n25\nOther\n12345\nTest Express\n"
            "Chennai\nBengaluru\n25:12:2026\n350\nSL\n250.00\n" +
            std::to_string(CNF) +
            "\nRESERVED\n1\n0\nBOOKING_END\n";
    }
};

TEST_F(ReservedBookingTest, GeneratePNRProducesNonEmptyValue) {
    char pnr[20] = {};
    generateBookingPNR(pnr);
    EXPECT_STRNE(pnr, "");
}

TEST_F(ReservedBookingTest, GeneratePNRFitsBuffer) {
    char pnr[20] = {};
    generateBookingPNR(pnr);
    EXPECT_LT(std::strlen(pnr), sizeof(pnr));
}

TEST_F(ReservedBookingTest, ConsecutivePNRsAreUnique) {
    char first[20] = {};
    char second[20] = {};

    generateBookingPNR(first);
    generateBookingPNR(second);

    EXPECT_STRNE(first, second);
}

TEST_F(ReservedBookingTest, GeneratedPNRDoesNotDuplicateExistingBooking) {
    char existingPNR[20] = {};
    generateBookingPNR(existingPNR);

    addBooking(makeBooking(existingPNR));

    char newPNR[20] = {};
    generateBookingPNR(newPNR);

    EXPECT_STRNE(newPNR, existingPNR);
}

TEST_F(ReservedBookingTest, ViewBookingsForUnknownPassengerDoesNotCrash) {
    EXPECT_NO_THROW(viewMyBookings(999999));
}

TEST_F(ReservedBookingTest, ViewBookingsWithEmptyListDoesNotCrash) {
    EXPECT_NO_THROW(viewMyBookings(123));
}

TEST_F(ReservedBookingTest, ViewBookingsForMatchingPassenger) {
    addBooking(makeBooking());
    EXPECT_NO_THROW(viewMyBookings(1));
}

TEST_F(ReservedBookingTest, SaveEmptyBookingList) {
    EXPECT_EQ(saveBookingData(), SUCCESS);
}

TEST_F(ReservedBookingTest, SaveNonEmptyBookingList) {
    addBooking(makeBooking());
    EXPECT_EQ(saveBookingData(), SUCCESS);
}

TEST_F(ReservedBookingTest, PrintReservedTicketDoesNotCrash) {
    Booking* b = makeBooking();
    ASSERT_NE(b, nullptr);

    EXPECT_NO_THROW(printBookingTicket(b));

    std::free(b);
}

TEST_F(ReservedBookingTest, PrintUnreservedTicketDoesNotCrash) {
    Booking* b = makeBooking();
    ASSERT_NE(b, nullptr);

    std::snprintf(
        b->bookingType,
        sizeof(b->bookingType),
        "UNRESERVED"
    );
    std::snprintf(
        b->className,
        sizeof(b->className),
        "UNRES"
    );
    b->pnrNumber[0] = '\0';
    b->adults = 2;
    b->children = 1;

    EXPECT_NO_THROW(printBookingTicket(b));

    std::free(b);
}

TEST_F(ReservedBookingTest, SaveAndReloadValidBooking) {
    addBooking(makeBooking());

    ASSERT_EQ(saveBookingData(), SUCCESS);

    clearBookings();
    loadBookingData();

    ASSERT_NE(bookingHead, nullptr);
    EXPECT_STREQ(bookingHead->pnrNumber, "PNR00000001");
    EXPECT_EQ(bookingHead->passengerId, 1);
    EXPECT_EQ(bookingHead->trainNumber, 12345);
    EXPECT_FLOAT_EQ(bookingHead->fare, 250.0f);
}

TEST_F(ReservedBookingTest, LoadMissingBookingFileDoesNotCrash) {
    std::remove("bookings.txt");
    EXPECT_NO_THROW(loadBookingData());
}

TEST_F(ReservedBookingTest, LoadFileWithoutBookingMarkers) {
    writeTextFile("bookings.txt", "Some unrelated text\nAnother line\n");

    EXPECT_NO_THROW(loadBookingData());
    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(ReservedBookingTest, LoadMalformedBookingRecord) {
    writeTextFile(
        "bookings.txt",
        "BOOKING_START\nPNR123\n1\nIncomplete record\n"
    );

    EXPECT_NO_THROW(loadBookingData());
}

TEST_F(ReservedBookingTest, LoadInvalidBookingRecord) {
    std::string record = validRecord();
    const std::string marker = "\n350\n";

    const std::size_t pos = record.find(marker);
    ASSERT_NE(pos, std::string::npos);

    // Distance must be positive for a valid record.
    record.replace(pos, marker.size(), "\n0\n");

    writeTextFile("bookings.txt", record);

    EXPECT_NO_THROW(loadBookingData());
    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(ReservedBookingTest, LoadOldRecordWithoutBookingType) {
    // Older file format ends after ticketStatus.
    std::string record =
        "BOOKING_START\nPNR00000077\n1\nTest Passenger\n25\nOther\n"
        "12345\nTest Express\nChennai\nBengaluru\n25:12:2026\n"
        "350\nSL\n250.00\n" + std::to_string(CNF) +
        "\nBOOKING_END\n";

    writeTextFile("bookings.txt", record);

    EXPECT_NO_THROW(loadBookingData());

    // The record is accepted only if its date and fields pass validation.
    if (bookingHead != nullptr) {
        EXPECT_STREQ(bookingHead->bookingType, "RESERVED");
    }
}

TEST_F(ReservedBookingTest, InitializeBookingModuleDoesNotCrash) {
    EXPECT_NO_THROW(initializeBookingModule());
}

TEST_F(ReservedBookingTest, CancelledStatusPrintsTicket) {
    Booking* b = makeBooking();
    ASSERT_NE(b, nullptr);

    b->ticketStatus = CANCELLED;

    EXPECT_NO_THROW(printBookingTicket(b));

    std::free(b);
}

TEST_F(ReservedBookingTest, RACStatusPrintsTicket) {
    Booking* b = makeBooking();
    ASSERT_NE(b, nullptr);

    b->ticketStatus = RAC;

    EXPECT_NO_THROW(printBookingTicket(b));

    std::free(b);
}

TEST_F(ReservedBookingTest, WaitingListStatusPrintsTicket) {
    Booking* b = makeBooking();
    ASSERT_NE(b, nullptr);

    b->ticketStatus = WL;

    EXPECT_NO_THROW(printBookingTicket(b));

    std::free(b);
}


TEST(BookingFlow, MissingPassengerReturnsWithoutBooking) {
    passengerHead = nullptr;

    BookingInput input("");
    bookTicket(999);

    EXPECT_EQ(bookingHead, nullptr);
}

TEST(BookingFlow, SameSourceAndDestinationIsRejected) {
    addBookingTestData(5, 2, 0, 5);

    BookingInput input("Chennai\nChennai\n");
    bookTicket(101);

    EXPECT_EQ(bookingHead, nullptr);
}

TEST(BookingFlow, NoScheduledTrainReturnsWithoutBooking) {
    addBookingTestData(5, 2, 0, 5);
    scheduleHead = nullptr;

    BookingInput input(
        "Chennai\nBengaluru\n25:12:2026\n"
    );
    bookTicket(101);

    EXPECT_EQ(bookingHead, nullptr);
}

TEST(BookingFlow, UserCanReturnAtTrainSelection) {
    addBookingTestData(5, 2, 0, 5);

    BookingInput input(
        "Chennai\nBengaluru\n25:12:2026\n0\n"
    );
    bookTicket(101);

    EXPECT_EQ(bookingHead, nullptr);
}

TEST(BookingFlow, UnsupportedClassCanBeRetried) {
    addBookingTestData(5, 2, 0, 5);

    BookingInput input(
        "Chennai\nBengaluru\n25:12:2026\n"
        "12345\nINVALID\n0\n"
    );
    bookTicket(101);

    EXPECT_EQ(bookingHead, nullptr);
}

TEST(BookingFlow, FailedPaymentDoesNotCreateBooking) {
    addBookingTestData(5, 2, 0, 5);

    BookingInput input(
        "Chennai\nBengaluru\n25:12:2026\n"
        "12345\nSL\n1\n1\n1\n"
    );
    bookTicket(101);

    EXPECT_EQ(bookingHead, nullptr);
    EXPECT_EQ(availabilityHead->availableSeats, 5);
}

TEST(BookingFlow, ConfirmedBookingCanBeCreated) {
    addBookingTestData(5, 2, 0, 5);

    BookingInput input(
        "Chennai\nBengaluru\n25:12:2026\n"
        "12345\nSL\n700\n"
    );
    bookTicket(101);

    ASSERT_NE(bookingHead, nullptr);
    EXPECT_EQ(bookingHead->passengerId, 101);
    EXPECT_EQ(bookingHead->ticketStatus, CNF);
    EXPECT_EQ(availabilityHead->availableSeats, 4);
}

TEST_F(ReservedBookingTest, RejectsEmptyPNR) {
    std::string record = validRecord();
    const auto pos = record.find("PNR00000001");
    ASSERT_NE(pos, std::string::npos);
    record.replace(pos, std::strlen("PNR00000001"), "");

    writeTextFile("bookings.txt", record);
    loadBookingData();

    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(ReservedBookingTest, RejectsInvalidPassengerId) {
    std::string record = validRecord();
    const auto pos = record.find("\n1\n");
    ASSERT_NE(pos, std::string::npos);
    record.replace(pos, 3, "\n0\n");

    writeTextFile("bookings.txt", record);
    loadBookingData();

    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(ReservedBookingTest, RejectsInvalidTrainNumber) {
    std::string record = validRecord();
    const auto pos = record.find("\n12345\n");
    ASSERT_NE(pos, std::string::npos);
    record.replace(pos, std::strlen("\n12345\n"), "\n0\n");

    writeTextFile("bookings.txt", record);
    loadBookingData();

    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(ReservedBookingTest, RejectsInvalidFare) {
    std::string record = validRecord();
    const auto pos = record.find("\n250.00\n");
    ASSERT_NE(pos, std::string::npos);
    record.replace(pos, std::strlen("\n250.00\n"), "\n0.00\n");

    writeTextFile("bookings.txt", record);
    loadBookingData();

    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(ReservedBookingTest, RejectsInvalidTicketStatus) {
    std::string record = validRecord();
    const auto pos = record.find(
        "\n" + std::to_string(CNF) + "\nRESERVED\n"
    );
    ASSERT_NE(pos, std::string::npos);

    record.replace(
        pos,
        std::to_string(CNF).size() + 2 + std::strlen("RESERVED\n"),
        "\n999\nRESERVED\n"
    );

    writeTextFile("bookings.txt", record);
    loadBookingData();

    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(ReservedBookingTest, RejectsUnsupportedClass) {
    std::string record = validRecord();
    const auto pos = record.find("\nSL\n");
    ASSERT_NE(pos, std::string::npos);
    record.replace(pos, 4, "\nINVALID\n");

    writeTextFile("bookings.txt", record);
    loadBookingData();

    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(ReservedBookingTest, RejectsInvalidGender) {
    std::string record = validRecord();
    const auto pos = record.find("\nOther\n");
    ASSERT_NE(pos, std::string::npos);
    record.replace(pos, 7, "\nUnknown\n");

    writeTextFile("bookings.txt", record);
    loadBookingData();

    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(ReservedBookingTest, RejectsInvalidJourneyDate) {
    std::string record = validRecord();
    const auto pos = record.find("25:12:2026");
    ASSERT_NE(pos, std::string::npos);
    record.replace(pos, std::strlen("25:12:2026"), "99:99:9999");

    writeTextFile("bookings.txt", record);
    loadBookingData();

    EXPECT_EQ(bookingHead, nullptr);
}

TEST_F(ReservedBookingTest, RejectsDuplicatePNR) {
    addBooking(makeBooking("PNR00000001"));
    writeTextFile("bookings.txt", validRecord("PNR00000001"));

    loadBookingData();

    // The existing record remains; the duplicate is not loaded.
    ASSERT_NE(bookingHead, nullptr);
    EXPECT_EQ(bookingHead->next, nullptr);
}

TEST(BookingFlow, CreatesRACBooking) {
    addBookingTestData(0, 2, 0, 5);

    BookingInput input(
        "Chennai\nBengaluru\n25:12:2026\n"
        "12345\nSL\n700\n"
    );

    bookTicket(101);

    ASSERT_NE(bookingHead, nullptr);
    EXPECT_EQ(bookingHead->ticketStatus, RAC);
    EXPECT_EQ(availabilityHead->availableRAC, 1);
}

TEST(BookingFlow, CreatesWaitingListBooking) {
    addBookingTestData(0, 0, 0, 5);

    BookingInput input(
        "Chennai\nBengaluru\n25:12:2026\n"
        "12345\nSL\n700\n"
    );

    bookTicket(101);

    ASSERT_NE(bookingHead, nullptr);
    EXPECT_EQ(bookingHead->ticketStatus, WL);
    EXPECT_EQ(availabilityHead->availableWL, 1);
}

TEST(BookingFlow, RejectsBookingWhenAllAvailabilityIsExhausted) {
    // Clear bookings left by previous tests.
    while (bookingHead != nullptr) {
        Booking* next = bookingHead->next;
        std::free(bookingHead);
        bookingHead = next;
    }

    addBookingTestData(0, 0, 5, 5);

    BookingInput input(
        "Chennai\nBengaluru\n25:12:2026\n"
        "12345\nSL\n"
    );

    bookTicket(101);

    EXPECT_EQ(bookingHead, nullptr);
}

TEST(BookingFlow, PaymentRetryEventuallySucceeds) {
    addBookingTestData(5, 2, 0, 5);

    BookingInput input(
        "Chennai\nBengaluru\n25:12:2026\n"
        "12345\nSL\n"
        "100\n700\n"
    );

    bookTicket(101);

    ASSERT_NE(bookingHead, nullptr);
    EXPECT_EQ(bookingHead->ticketStatus, CNF);
    EXPECT_EQ(availabilityHead->availableSeats, 4);
}

TEST_F(ReservedBookingTest, GeneratePNRRetriesWhenDuplicateExists) {
    Booking* existing = static_cast<Booking*>(std::calloc(1, sizeof(Booking)));
    ASSERT_NE(existing, nullptr);

    char generated[20];
    generateBookingPNR(generated);

    std::snprintf(existing->pnrNumber, sizeof(existing->pnrNumber), "%s", generated);
    existing->next = bookingHead;
    if (bookingHead != nullptr) {
        bookingHead->prev = existing;
    }
    bookingHead = existing;

    char nextPNR[20];
    generateBookingPNR(nextPNR);

    EXPECT_STRNE(nextPNR, existing->pnrNumber);
}

TEST_F(ReservedBookingTest, RejectsUnsupportedClassDuringLoad) {
    std::ofstream file("bookings.txt");
    ASSERT_TRUE(file.is_open());

    file << "BOOKING_START\n"
         << "PNR12345678\n"
         << "101\n"
         << "Test Passenger\n"
         << "25\n"
         << "Male\n"
         << "12345\n"
         << "Test Express\n"
         << "Chennai\n"
         << "Bengaluru\n"
         << "25:12:2026\n"
         << "350\n"
         << "INVALID\n"
         << "700.00\n"
         << "0\n"
         << "RESERVED\n"
         << "1\n"
         << "0\n"
         << "BOOKING_END\n";
    file.close();

    loadBookingData();

    EXPECT_EQ(bookingHead, nullptr);
}

