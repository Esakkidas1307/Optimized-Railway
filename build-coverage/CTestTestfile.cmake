# CMake generated Testfile for 
# Source directory: /workspaces/Optimized-Railway
# Build directory: /workspaces/Optimized-Railway/build-coverage
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/workspaces/Optimized-Railway/build-coverage/admin_auth_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-coverage/adminmenu_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-coverage/train_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-coverage/usermenu_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-coverage/main_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-coverage/passenger_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-coverage/pnr_status_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-coverage/reallocation_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-coverage/reserved_booking_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-coverage/railway_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-coverage/cancellation_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-coverage/trainschedule_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-coverage/train_search_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-coverage/unreserved_booking_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build-coverage/user_tests[1]_include.cmake")
add_test([=[optimization_index_tests]=] "/workspaces/Optimized-Railway/build-coverage/optimization_index_tests")
set_tests_properties([=[optimization_index_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/workspaces/Optimized-Railway/CMakeLists.txt;23;add_test;/workspaces/Optimized-Railway/CMakeLists.txt;0;")
subdirs("_deps/googletest-build")
