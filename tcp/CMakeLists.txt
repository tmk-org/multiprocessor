cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "tcp")

project(${PROJECT_NAME}
    DESCRIPTION "Network communication library"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/tcp"
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
    include/tcp/tcp_process.h
    #sources
    src/tcp_process.c
)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Doesn't work without defining parseReply and getRequest !!!

#add_executable(tcp_server examples/server.c)
#target_link_libraries(tcp_server tcp)

#add_executable(tcp_client examples/client.c)
#target_link_libraries(tcp_client tcp)

###############################################################################
# Tests
###############################################################################

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

set(PROJECT_TESTS ${PROJECT_NAME}-tests)

#add_executable(${PROJECT_TESTS}
#    tests/TcpTests.cpp
#)

#add_executable(${PROJECT_TESTS}
#    tests/tcp_tests.c
#)

#target_link_libraries(${PROJECT_TESTS}
#    ${GTEST_BOTH_LIBRARIES} pthread
#    ${PROJECT_NAME}
#)

#add_test(${PROJECT_TESTS} ${PROJECT_TESTS})
#add_test(NAME ${PROJECT_TESTS} COMMAND $<TARGET_FILE:${PROJECT_TESTS}>
