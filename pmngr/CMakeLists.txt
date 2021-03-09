cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "pmngr")

project(${PROJECT_NAME}
    DESCRIPTION "Process manager communication library"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/pmngr"
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
    #header
    include/pmngr/signal_reaction.h
    include/pmngr/process_mngr.h
    include/pmngr/process_api.h
    #sources
    src/signal_reaction.c
    src/process_mngr.c
)

target_link_libraries(${PROJECT_NAME} pthread)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

#add_executable(pmngr_server examples/pmngr_server.c)
#target_link_libraries(pmngr_server ${PROJECT_NAME})

#add_executable(pmngr_empty examples/pmngr_empty.c)
#target_link_libraries(pmngr_empty ${PROJECT_NAME})

###############################################################################
# Tests
###############################################################################

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

set(PROJECT_TESTS ${PROJECT_NAME}-tests)

#add_executable(${PROJECT_TESTS}
#    tests/PmngrTests.cpp
#)

#add_executable(${PROJECT_TESTS}
#    tests/pmngr_tests.c
#)

#target_link_libraries(${PROJECT_TESTS}
#    ${GTEST_BOTH_LIBRARIES} pthread
#    ${PROJECT_NAME}
#)

#add_test(${PROJECT_TESTS} ${PROJECT_TESTS})
#add_test(NAME ${PROJECT_TESTS} COMMAND $<TARGET_FILE:${PROJECT_TESTS}>
