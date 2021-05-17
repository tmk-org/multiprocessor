cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "pipeline")

project(${PROJECT_NAME}
    DESCRIPTION "Pipeline example"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/pipeline"
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
include_directories(${CMAKE_SOURCE_DIR}/../tcp/include)
include_directories(${CMAKE_SOURCE_DIR}/../pmngr/include)
include_directories(${CMAKE_SOURCE_DIR}/../config/include)
include_directories(${CMAKE_SOURCE_DIR}/../command_server/include)

add_library(${PROJECT_NAME} SHARED
    #header
    include/pipeline/pipeline.h
    #sources
    src/pipeline_server.c
    src/pipeline_client.c
)

target_link_libraries(${PROJECT_NAME} misc shmem pmngr tcp config)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -g")

add_executable(pfirst components/first.c)
target_link_libraries(pfirst ${PROJECT_NAME} shmem pmngr pipeline)

add_executable(pmiddle components/middle.c)
target_link_libraries(pmiddle ${PROJECT_NAME} shmem pmngr pipeline)

add_executable(pserver components/server.c)
target_link_libraries(pserver ${PROJECT_NAME} shmem pmngr pipeline tcp cmdsrv)

add_executable(pmanager components/manager.c)
target_link_libraries(pmanager tcp)

###############################################################################
# Tests
###############################################################################

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

set(PROJECT_TESTS ${PROJECT_NAME}-tests)

#add_executable(${PROJECT_TESTS}
#    tests/ModelTests.cpp
#)

#add_executable(${PROJECT_TESTS}
#    tests/model_tests.c
#)

#target_link_libraries(${PROJECT_TESTS}
#    ${GTEST_BOTH_LIBRARIES} pthread rt
#    ${PROJECT_NAME}
#)

#add_test(${PROJECT_TESTS} ${PROJECT_TESTS})
#add_test(NAME ${PROJECT_TESTS} COMMAND $<TARGET_FILE:${PROJECT_TESTS}>
