cmake_minimum_required(VERSION 3.20)

project(RailwayReservationTests LANGUAGES C CXX)

# --------------------------------------------------
# Language standards
# --------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(RAILWAY_BUILD_TESTS "Build Google Test suites" ON)
find_package(Threads REQUIRED)
enable_testing()

add_executable(optimization_index_tests
    optimization_index_test.c optimization_support.c
)
target_include_directories(optimization_index_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(optimization_index_tests PRIVATE Threads::Threads)
add_test(NAME optimization_index_tests COMMAND optimization_index_tests)

add_executable(optimization_benchmark
    optimization_benchmark.c optimization_support.c
)
target_include_directories(optimization_benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(optimization_benchmark PRIVATE Threads::Threads)

# Standalone application target. This can be configured without downloading Google Test.
add_executable(railway_app
    main.c admin_auth.c adminmenu.c availability.c cancellation.c
    date_time.c passenger.c pnr_status.c reallocation.c refund.c
    reserved_booking.c train.c train_search.c trainschedule.c
    unreserved_booking.c user.c usermenu.c utils.c
    optimization_runtime.c optimization_support.c
)
target_include_directories(railway_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(railway_app PRIVATE Threads::Threads m)

if(NOT RAILWAY_BUILD_TESTS)
    return()
endif()

# --------------------------------------------------
# Google Test
# --------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

include(GoogleTest)

# --------------------------------------------------
# Admin authentication tests
# --------------------------------------------------
add_executable(admin_auth_tests
    admin_auth.c
    date_time.c
    admin_auth_test_support.c
    test_admin_auth.cpp
)

target_include_directories(admin_auth_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(admin_auth_tests PRIVATE
    GTest::gtest_main
)

set_target_properties(admin_auth_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(admin_auth_tests PRIVATE --coverage -O0 -g)
    target_link_options(admin_auth_tests PRIVATE --coverage)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(admin_auth_tests PRIVATE --coverage -O0 -g)
    target_link_options(admin_auth_tests PRIVATE --coverage)
endif()

gtest_discover_tests(admin_auth_tests)

# --------------------------------------------------
# Admin menu tests
# --------------------------------------------------
add_executable(adminmenu_tests
    adminmenu.c
    test_adminmenu.cpp
    adminmenu_test_support.c
)

target_include_directories(adminmenu_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(adminmenu_tests PRIVATE
    GTest::gtest_main
)

set_target_properties(adminmenu_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(adminmenu_tests PRIVATE --coverage -O0 -g)
    target_link_options(adminmenu_tests PRIVATE --coverage)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(adminmenu_tests PRIVATE --coverage -O0 -g)
    target_link_options(adminmenu_tests PRIVATE --coverage)
endif()

gtest_discover_tests(adminmenu_tests)


set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

# --------------------------------------------------
# Coverage configuration
# --------------------------------------------------
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(COVERAGE_COMPILE_FLAGS --coverage -O0 -g)
    set(COVERAGE_LINK_FLAGS --coverage)
endif()

# --------------------------------------------------
# Core library for existing tests
# --------------------------------------------------
add_library(railway_test_core STATIC
    utils.c
    date_time.c
    refund.c
    availability.c
    train.c
    train_search.c
    trainschedule.c
    reallocation.c
)
target_include_directories(railway_test_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
)


# --------------------------------------------------
# Train module tests
# --------------------------------------------------
add_executable(train_tests
    test_train.cpp
)

target_include_directories(train_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(train_tests PRIVATE
    railway_test_core
    GTest::gtest_main
)

set_target_properties(train_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(train_tests PRIVATE --coverage -O0 -g)
    target_link_options(train_tests PRIVATE --coverage)
endif()

gtest_discover_tests(train_tests)



if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(railway_test_core
        PRIVATE
            ${COVERAGE_COMPILE_FLAGS}
    )

    target_link_options(railway_test_core
        INTERFACE
            ${COVERAGE_LINK_FLAGS}
    )
endif()

add_executable(usermenu_tests
    usermenu.c
    date_time.c
    usermenu_test_support.c
    test_usermenu.cpp
)

target_include_directories(usermenu_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(usermenu_tests PRIVATE
    GTest::gtest_main
)

set_target_properties(usermenu_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(usermenu_tests PRIVATE --coverage -O0 -g)
    target_link_options(usermenu_tests PRIVATE --coverage)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(usermenu_tests PRIVATE --coverage -O0 -g)
    target_link_options(usermenu_tests PRIVATE --coverage)
endif()

gtest_discover_tests(usermenu_tests)


include(GoogleTest)



include(GoogleTest)

add_library(main_test_entry OBJECT
    main.c
)

target_compile_definitions(main_test_entry PRIVATE
    main=railway_app_main
)

target_include_directories(main_test_entry PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_executable(main_tests
    $<TARGET_OBJECTS:main_test_entry>
    test_main.cpp
    main_test_support.c
)

target_include_directories(main_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(main_tests PRIVATE
    GTest::gtest_main
)

set_target_properties(main_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(main_test_entry PRIVATE
        --coverage -O0 -g
    )
    target_compile_options(main_tests PRIVATE
        --coverage -O0 -g
    )
    target_link_options(main_tests PRIVATE --coverage)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(main_tests PRIVATE
        --coverage -O0 -g
    )
    target_link_options(main_tests PRIVATE --coverage)
endif()

gtest_discover_tests(main_tests)


include(GoogleTest)

add_library(passenger_test_module OBJECT passenger.c)

target_include_directories(passenger_test_module PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(passenger_tests
    test_passenger.cpp
    $<TARGET_OBJECTS:passenger_test_module>
)

target_include_directories(passenger_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(passenger_tests PRIVATE
    GTest::gtest_main
)

target_compile_options(passenger_test_module PRIVATE
    --coverage -O0 -g
)

target_compile_options(passenger_tests PRIVATE
    --coverage -O0 -g
)

target_link_options(passenger_tests PRIVATE
    --coverage
)

gtest_discover_tests(passenger_tests)


add_library(pnr_status_test_module OBJECT pnr_status.c)

target_include_directories(
    pnr_status_test_module
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(pnr_status_tests
    test_pnr_status.cpp
    pnr_status_test_support.c
    $<TARGET_OBJECTS:pnr_status_test_module>
)

target_include_directories(
    pnr_status_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    pnr_status_tests
    PRIVATE GTest::gtest_main
)

target_compile_options(
    pnr_status_test_module
    PRIVATE --coverage -O0 -g
)

target_compile_options(
    pnr_status_tests
    PRIVATE --coverage -O0 -g
)

target_link_options(
    pnr_status_tests
    PRIVATE --coverage
)

gtest_discover_tests(pnr_status_tests)


add_library(reallocation_test_module OBJECT reallocation.c)

target_include_directories(
    reallocation_test_module
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(reallocation_tests
    test_reallocation.cpp
    $<TARGET_OBJECTS:reallocation_test_module>
)

target_include_directories(
    reallocation_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    reallocation_tests
    PRIVATE GTest::gtest_main
)

target_compile_options(
    reallocation_test_module
    PRIVATE --coverage -O0 -g
)

target_compile_options(
    reallocation_tests
    PRIVATE --coverage -O0 -g
)

target_link_options(reallocation_tests PRIVATE --coverage)

gtest_discover_tests(reallocation_tests)

add_library(reserved_booking_test_module OBJECT reserved_booking.c)

target_include_directories(reserved_booking_test_module
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(reserved_booking_test_module
    PRIVATE --coverage -O0 -g
)

add_executable(reserved_booking_tests
    test_reserved_booking.cpp
    reserved_booking.c
    passenger.c
)
target_include_directories(reserved_booking_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(reserved_booking_tests
    PRIVATE
    GTest::gtest_main
    railway_test_core
)


target_compile_options(reserved_booking_tests PRIVATE
    --coverage
)

target_link_options(reserved_booking_tests PRIVATE
    --coverage
)

gtest_discover_tests(reserved_booking_tests)


# --------------------------------------------------
# Tests: utils, refund, availability
# IMPORTANT:
# Do not add test_cancellation.cpp here.
# It contains mocks that conflict with real functions
# in railway_test_core.
# --------------------------------------------------
add_executable(railway_tests
    test_utils.cpp
    test_refund.cpp
    test_availability.cpp
)

target_include_directories(railway_tests
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(railway_tests
    PRIVATE
        railway_test_core
        GTest::gtest_main
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(railway_tests
        PRIVATE
            ${COVERAGE_COMPILE_FLAGS}
    )

    target_link_options(railway_tests
        PRIVATE
            ${COVERAGE_LINK_FLAGS}
    )
endif()

gtest_discover_tests(railway_tests)

# --------------------------------------------------
# Separate cancellation test executable
# Compiles cancellation.c with its test doubles.
# --------------------------------------------------
add_executable(cancellation_tests
    cancellation.c
    test_cancellation.cpp
)

target_include_directories(cancellation_tests
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(cancellation_tests
    PRIVATE
        GTest::gtest_main
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cancellation_tests
        PRIVATE
            ${COVERAGE_COMPILE_FLAGS}
    )

    target_link_options(cancellation_tests
        PRIVATE
            ${COVERAGE_LINK_FLAGS}
    )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cancellation_tests
        PRIVATE
            ${COVERAGE_COMPILE_FLAGS}
    )

    target_link_options(cancellation_tests
        PRIVATE
            ${COVERAGE_LINK_FLAGS}
    )
endif()

gtest_discover_tests(cancellation_tests)

# Train Schedule GoogleTest target
add_executable(trainschedule_tests
    test_trainschedule.cpp
)

target_link_libraries(trainschedule_tests
    PRIVATE
    railway_test_core
    GTest::gtest_main
)

gtest_discover_tests(trainschedule_tests)
# Runtime/index support is also needed by existing tests that compile modules.
target_sources(railway_test_core PRIVATE optimization_runtime.c optimization_support.c)
target_link_libraries(railway_test_core PUBLIC Threads::Threads)
target_sources(passenger_test_module PRIVATE optimization_runtime.c optimization_support.c)
target_link_libraries(passenger_tests PRIVATE Threads::Threads)
target_sources(main_tests PRIVATE optimization_runtime.c)
target_link_libraries(main_tests PRIVATE Threads::Threads)
