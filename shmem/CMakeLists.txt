cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "shmem")

project(${PROJECT_NAME}
    DESCRIPTION "Shared memory communication library"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/shmem"
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
    include/shmem/shared_memory.h
    include/shmem/shared_memory_map.h
    #sources
    src/shared_memory.c
)

target_link_libraries(${PROJECT_NAME} rt)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(shm_server examples/server.c)
target_link_libraries(shm_server ${PROJECT_NAME})

add_executable(shm_reader examples/reader.c)
target_link_libraries(shm_reader ${PROJECT_NAME})

add_executable(shm_writer examples/writer.c)
target_link_libraries(shm_writer ${PROJECT_NAME})

###############################################################################
# Tests
###############################################################################

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

set(PROJECT_TESTS ${PROJECT_NAME}-tests)

#add_executable(${PROJECT_TESTS}
#    tests/ShmemTests.cpp
#)

#add_executable(${PROJECT_TESTS}
#    tests/shm_tests.c
#)

#target_link_libraries(${PROJECT_TESTS}
#    ${GTEST_BOTH_LIBRARIES} pthread
#    ${PROJECT_NAME}
#)

#add_test(${PROJECT_TESTS} ${PROJECT_TESTS})
#add_test(NAME ${PROJECT_TESTS} COMMAND $<TARGET_FILE:${PROJECT_TESTS}>
