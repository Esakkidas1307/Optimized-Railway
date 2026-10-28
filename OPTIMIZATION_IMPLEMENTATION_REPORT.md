# Railway Reservation System — Optimization Integration Report

## Implemented

1. **Five-minute input inactivity timeout** in the shared line-input function. Timeout returns control to the main entry point for shutdown.
2. **Incomplete-operation persistence protection:** data is saved at the existing successful-operation save points. A timeout during an unfinished operation does not mark that operation's collection dirty, so the incomplete in-memory edit is not written by timeout cleanup.
3. **Per-collection dirty flags** for trains, schedules, passengers, running inventory, users, and bookings.
4. **Conditional retry saving:** a collection is retried only while dirty; its flag is cleared only after the save callback reports success.
5. **Pthread autosave scheduler and mutex protection:** the worker thread schedules periodic checks and updates runtime state under a mutex. To avoid races with DLL traversal, save callbacks execute on the main thread at safe input-wait points, not concurrently with DLL mutations.
6. **Prompt booking/cancellation persistence:** existing booking, cancellation, and inventory save calls are retained. The existing booking and inventory writers use temporary files and rename; other core writers now use temporary files and check write/flush/close/rename errors.
7. **Shutdown/timeout flush:** the scheduler is stopped and joined, then remaining dirty collections are retried. SIGINT/SIGTERM are converted to a safe shutdown request when the main thread next reaches its input wait loop. SIGKILL and power failure cannot be handled by an in-process cleanup handler.
8. **Sorted train, schedule, and passenger storage:** saved `trains.txt`, `trainschedule.txt`, and `passengers.txt` are written in key order. In-memory sorted pointer indexes use Merge Sort.
9. **Binary search:** train-number lookup, passenger-ID lookup, and schedule lookup by train number/date use sorted pointer indexes. Multiple schedule records for one train number are ordered by journey date and can be scanned as a matching range.

## Added files

- `optimization_runtime.c/.h`: dirty-state tracking, pthread scheduler, timeout/interruption state, and safe-point save servicing.
- `optimization_support.c/.h`: generic Merge-Sort pointer index and binary-search operations.
- `tests/optimization_index_test.c`: small correctness test for sorted index and binary lookup.
- `tests/optimization_benchmark.c`: illustrative sequential-vs-index lookup benchmark.
- `Makefile`: standalone app build.

## Build and verification performed

- GCC strict build: `-std=c11 -Wall -Wextra -Wconversion -Wsign-conversion -Wpedantic -pthread` — passed with no warnings.
- Clang strict build with the same warning flags — passed after adding final newlines to source/header files.
- CMake application-only build: `cmake -S . -B build-app -DRAILWAY_BUILD_TESTS=OFF && cmake --build build-app -j2` — passed.
- `ctest --test-dir build-app --output-on-failure` — optimization index test passed (1/1).
- Application smoke test with input `0` — exited successfully.
- Benchmark sample (10,000 lookups on 10,000 records) ran successfully; observed times are machine/build dependent and should not be treated as a guaranteed speedup.

## Not verified

- The full pre-existing Google Test suite was not run because the archive did not include a Google Test source checkout and CMake could not configure its FetchContent dependency in this environment.
- Cppcheck/MISRA analysis was not run because `cppcheck` is not installed in this environment.
- The five-minute timeout was not tested by waiting five minutes; the path was compile-checked and the ordinary exit path was smoke-tested.
- Abrupt power loss and SIGKILL cannot guarantee persistence. For that level of durability, use a transaction journal/database and operating-system sync calls.
