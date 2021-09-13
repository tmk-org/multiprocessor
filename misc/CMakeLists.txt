cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "misc")

project(${PROJECT_NAME}
    DESCRIPTION "Usefull utilities in the one library"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/misc"
    LANGUAGES C CXX)

###############################################################################
# Include CMake dependencies
###############################################################################

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

###############################################################################
# Options
###############################################################################

###############################################################################
# Find packages
###############################################################################

###############################################################################
# Add sources
###############################################################################

#include(GenerateVersion)

add_library(${PROJECT_NAME} SHARED
    #headers
    include/misc/log.h
    include/misc/list.h
    include/misc/ring_buffer.h
    #sources
    src/ring_buffer.c
)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

#add_executable(ring_example examples/ring_example.c)
#target_link_libraries(misc ${PROJECT_NAME})

#add_executable(list_example examples/list_example.c)
#target_link_libraries(misc ${PROJECT_NAME})

###############################################################################
# Tests
###############################################################################

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

set(PROJECT_TESTS ${PROJECT_NAME}-tests)

#add_executable(${PROJECT_TESTS}
#    tests/MiscTests.cpp
#)

#add_executable(${PROJECT_TESTS}
#    tests/misc_tests.c
#)

#target_link_libraries(${PROJECT_TESTS}
#    ${GTEST_BOTH_LIBRARIES} pthread
#    ${PROJECT_NAME}
#)

#add_test(${PROJECT_TESTS} ${PROJECT_TESTS})
#add_test(NAME ${PROJECT_TESTS} COMMAND $<TARGET_FILE:${PROJECT_TESTS}>
