cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "messenger")

project(${PROJECT_NAME}
    DESCRIPTION "Network communication library"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/messenger"
    LANGUAGES C)

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
    include/${PROJECT_NAME}/messenger.h
    #sources
    src/messenger.c
)

target_link_libraries (${PROJECT_NAME} misc pthread)
target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(msg_server examples/server.c)
target_link_libraries(msg_server ${PROJECT_NAME})

add_executable(msg_client examples/client.c)
target_link_libraries(msg_client ${PROJECT_NAME})

add_executable(msg_proxy examples/proxy.c)
target_link_libraries(msg_proxy ${PROJECT_NAME})

###############################################################################
# Tests
###############################################################################

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

set(PROJECT_TESTS ${PROJECT_NAME}-tests)

#add_executable(${PROJECT_TESTS}
#    tests/msg_tests.c
#)

#target_link_libraries(${PROJECT_TESTS}
#    ${GTEST_BOTH_LIBRARIES} pthread
#    ${PROJECT_NAME}
#)

#add_test(${PROJECT_TESTS} ${PROJECT_TESTS})
#add_test(NAME ${PROJECT_TESTS} COMMAND $<TARGET_FILE:${PROJECT_TESTS}>
