# CMake generated Testfile for 
# Source directory: /workspaces/Optimized-Railway
# Build directory: /workspaces/Optimized-Railway/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/workspaces/Optimized-Railway/build/admin_auth_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build/adminmenu_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build/usermenu_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build/main_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build/passenger_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build/pnr_status_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build/reallocation_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build/reserved_booking_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build/railway_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build/cancellation_tests[1]_include.cmake")
include("/workspaces/Optimized-Railway/build/trainschedule_tests[1]_include.cmake")
add_test([=[optimization_index_tests]=] "/workspaces/Optimized-Railway/build/optimization_index_tests")
set_tests_properties([=[optimization_index_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/workspaces/Optimized-Railway/CMakeLists.txt;23;add_test;/workspaces/Optimized-Railway/CMakeLists.txt;0;")
subdirs("_deps/googletest-build")
