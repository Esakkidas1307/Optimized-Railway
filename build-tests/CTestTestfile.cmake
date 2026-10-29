# CMake generated Testfile for 
# Source directory: /workspaces/Optimized-Railway
# Build directory: /workspaces/Optimized-Railway/build-tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/workspaces/Optimized-Railway/build-tests/admin_auth_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-tests/adminmenu_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-tests/usermenu_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-tests/main_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-tests/passenger_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-tests/pnr_status_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-tests/reallocation_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-tests/reserved_booking_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-tests/railway_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-tests/cancellation_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-tests/trainschedule_tests[1]_include.cmake")
add_test([=[optimization_index_tests]=] "/workspaces/Optimized-Railway/build-tests/optimization_index_tests")
set_tests_properties([=[optimization_index_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/workspaces/Optimized-Railway/CMakeLists.txt;23;add_test;/workspaces/Optimized-Railway/CMakeLists.txt;0;")
subdirs("_deps/googletest-build")
