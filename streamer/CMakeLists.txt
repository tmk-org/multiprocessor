cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "streamer")

project(${PROJECT_NAME}
    DESCRIPTION "Streamer/pipeline"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/streamer"
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

include_directories(${CMAKE_SOURCE_DIR}/../misc/include)
include_directories(${CMAKE_SOURCE_DIR}/../shmem/include)
include_directories(${CMAKE_SOURCE_DIR}/../pmngr/include)

add_library(${PROJECT_NAME} SHARED
    #header
    include/streamer/streamer.h
    include/streamer/pipeline.h
    #sources
    src/streamer.c
    src/pipeline.c
)

target_link_libraries(${PROJECT_NAME} -pthread misc shmem pmngr)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(capturer examples/capturer.c)
target_link_libraries(capturer ${PROJECT_NAME} shmem pmngr)

add_executable(filter examples/filter.c)
target_link_libraries(filter ${PROJECT_NAME} shmem pmngr)

add_executable(server examples/server.c)
target_link_libraries(server ${PROJECT_NAME} shmem pmngr)

###############################################################################
# Tests
###############################################################################

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

set(PROJECT_TESTS ${PROJECT_NAME}-tests)

#add_executable(${PROJECT_TESTS}
#    tests/StreamerTests.cpp
#    tests/PipelineTests.cpp
#)

#add_executable(${PROJECT_TESTS}
#    tests/streamer_tests.c
#    tests/pipeline_tests.c
#)

#target_link_libraries(${PROJECT_TESTS}
#    ${GTEST_BOTH_LIBRARIES} pthread rt
#    ${PROJECT_NAME}
#)

#add_test(${PROJECT_TESTS} ${PROJECT_TESTS})
#add_test(NAME ${PROJECT_TESTS} COMMAND $<TARGET_FILE:${PROJECT_TESTS}>
