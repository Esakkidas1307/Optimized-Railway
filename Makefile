# Railway Management System Makefile
# Uses the existing CMakeLists.txt to build C modules and their C++ Google Tests.
# Keep source/test linkage in CMakeLists.txt to avoid duplicate symbols and missing dependencies.

BUILD_DIR ?= build-coverage
CMAKE ?= cmake
CTEST ?= ctest
PYTHON ?= python3
GCOVR ?= $(PYTHON) -m gcovr

# CMake targets already defined by the project.
APP_TARGET ?= railway_app
USER_TEST_TARGET ?= user_tests
UNRESERVED_TEST_TARGET ?= unreserved_booking_tests

.PHONY: all configure app build test gtest user-test unreserved-test \
        coverage coverage-user coverage-unreserved html-user \
        sanitize valgrind helgrind cppcheck clean help

all: build

configure:
	$(CMAKE) -S . -B $(BUILD_DIR)

app: configure
	$(CMAKE) --build $(BUILD_DIR) --target $(APP_TARGET) -j2

build: configure
	$(CMAKE) --build $(BUILD_DIR) -j2

# Run every test registered with Google Test / CTest.
test gtest: build
	$(CTEST) --test-dir $(BUILD_DIR) --output-on-failure
    
# Run selected Google Test suites and report coverage for selected source files.
gtest-selected: configure
	$(CMAKE) --build $(BUILD_DIR) -j2
		$(CTEST) --test-dir $(BUILD_DIR) \
		-R 'AdminAuth|AdminMenu|Availability|Cancellation|Integration|Passenger|PnrStatus|Reallocation|Refund|ReservedBooking|UserMenu|Utils|User|TrainSearch|TrainSchedule' \
		--output-on-failure
		$(GCOVR) -r . $(BUILD_DIR) \
		--filter 'user\.c$$' \
		--filter 'train_search\.c$$' \
		--filter 'trainschedule\.c$$' \
		--filter 'tests/test_admin_auth\.cpp$$' \
		--filter 'tests/test_adminmenu\.cpp$$' \
		--filter 'tests/test_availability\.cpp$$' \
		--filter 'tests/test_cancellation\.cpp$$' \
		--filter 'tests/test_integration\.cpp$$' \
		--filter 'tests/test_main\.cpp$$' \
		--filter 'tests/test_passenger\.cpp$$' \
		--filter 'tests/test_pnr_status\.cpp$$' \
		--filter 'tests/test_reallocation\.cpp$$' \
		--filter 'tests/test_refund\.cpp$$' \
		--filter 'tests/test_reserved_booking\.cpp$$' \
		--filter 'tests/test_usermenu\.cpp$$' \
		--filter 'tests/test_utils\.cpp$$' \
		--print-summary


user-test: configure
	$(CMAKE) --build $(BUILD_DIR) --target $(USER_TEST_TARGET) -j2
	$(CTEST) --test-dir $(BUILD_DIR) -R 'UserModuleTest\.' --output-on-failure

unreserved-test: configure
	$(CMAKE) --build $(BUILD_DIR) --target $(UNRESERVED_TEST_TARGET) -j2
	$(CTEST) --test-dir $(BUILD_DIR) -R 'UnreservedBookingTest\.' --output-on-failure

# Run the user tests, then report coverage for user.c only.
coverage-user: user-test
	./$(BUILD_DIR)/$(USER_TEST_TARGET)
	$(GCOVR) -r . $(BUILD_DIR) \
		--gcov-object-directory $(BUILD_DIR)/CMakeFiles/$(USER_TEST_TARGET).dir \
		--filter 'user\.c$$' --print-summary

# Run unreserved booking tests, then report coverage for unreserved_booking.c only.
coverage-unreserved: unreserved-test
	./$(BUILD_DIR)/$(UNRESERVED_TEST_TARGET)
	$(GCOVR) -r . $(BUILD_DIR) \
		--gcov-object-directory $(BUILD_DIR)/CMakeFiles/$(UNRESERVED_TEST_TARGET).dir \
		--filter 'unreserved_booking\.c$$' --print-summary

# Overall coverage for the build. May include all instrumented modules.
coverage: build
	$(CTEST) --test-dir $(BUILD_DIR) --output-on-failure
	$(GCOVR) -r . $(BUILD_DIR) --print-summary

html-user: user-test
	./$(BUILD_DIR)/$(USER_TEST_TARGET)
	$(GCOVR) -r . $(BUILD_DIR) \
		--gcov-object-directory $(BUILD_DIR)/CMakeFiles/$(USER_TEST_TARGET).dir \
		--filter 'user\.c$$' \
		--html-details user-coverage.html
	@echo "Created user-coverage.html"

# AddressSanitizer + UndefinedBehaviorSanitizer build.
# Configure a separate build so sanitizer flags don't contaminate coverage artifacts.
sanitize:
	$(CMAKE) -S . -B build-sanitize \
		-DCMAKE_C_FLAGS="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" \
		-DCMAKE_CXX_FLAGS="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" \
		-DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
	$(CMAKE) --build build-sanitize -j2
	$(CTEST) --test-dir build-sanitize --output-on-failure

# Optional memory/thread checks. Supply one or more Google Test executable paths:
# make valgrind TEST_BINS="build-coverage/user_tests build-coverage/unreserved_booking_tests"
TEST_BINS ?= $(BUILD_DIR)/$(USER_TEST_TARGET) $(BUILD_DIR)/$(UNRESERVED_TEST_TARGET)

valgrind: build
	@command -v valgrind >/dev/null 2>&1 || { echo "valgrind is not installed"; exit 1; }
	@set -e; for bin in $(TEST_BINS); do \
		if [ -x "$$bin" ]; then \
			echo "== Valgrind: $$bin =="; \
			valgrind --leak-check=full --error-exitcode=1 "$$bin"; \
		else \
			echo "Skipping missing executable: $$bin"; \
		fi; \
	done

helgrind: build
	@command -v valgrind >/dev/null 2>&1 || { echo "valgrind is not installed"; exit 1; }
	@set -e; for bin in $(TEST_BINS); do \
		if [ -x "$$bin" ]; then \
			echo "== Helgrind: $$bin =="; \
			valgrind --tool=helgrind --error-exitcode=1 "$$bin"; \
		else \
			echo "Skipping missing executable: $$bin"; \
		fi; \
	done

# MISRA/static-analysis checks; requires cppcheck and a suitable MISRA addon/rules.
cppcheck:
	@command -v cppcheck >/dev/null 2>&1 || { echo "cppcheck is not installed"; exit 1; }
	cppcheck --enable=all --inconclusive --std=c99 \
		--suppress=missingIncludeSystem \
		-I. --error-exitcode=1 \
		*.c

clean:
	$(CMAKE) --build $(BUILD_DIR) --target clean
	rm -rf build-sanitize

help:
	@echo "Targets:"
	@echo "  make app                 Build railway_app"
	@echo "  make test / make gtest   Build and run all registered tests"
	@echo "  make user-test           Build/run UserModuleTest"
	@echo "  make unreserved-test     Build/run UnreservedBookingTest"
	@echo "  make coverage-user       Run user tests and user.c coverage"
	@echo "  make coverage-unreserved Run unreserved tests and module coverage"
	@echo "  make coverage            Run full suite and gcovr summary"
	@echo "  make sanitize            Build and test with ASan/UBSan"
	@echo "  make valgrind            Valgrind selected test executables"
	@echo "  make helgrind            Helgrind selected test executables"
	@echo "  make cppcheck            Static analysis of C source files"
	@echo "  Override BUILD_DIR=... or TEST_BINS=... as needed"
