cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "model")

project(${PROJECT_NAME}
    DESCRIPTION "Model of modules manager"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/model"
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

add_library(${PROJECT_NAME} SHARED
    #header
    include/model/model.h
    include/model/command_server.h
    #sources
    src/command_server.c
    src/model.c
)

target_link_libraries(${PROJECT_NAME} misc shmem pmngr tcp config)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -g")

add_executable(mcapturer components/capturer.c)
target_link_libraries(mcapturer ${PROJECT_NAME} shmem pmngr)

add_executable(mfilter components/filter.c)
target_link_libraries(mfilter ${PROJECT_NAME} shmem pmngr)

add_executable(mserver components/server.c)
target_link_libraries(mserver ${PROJECT_NAME} shmem pmngr model tcp)

add_executable(mmanager components/manager.c)
target_link_libraries(mmanager tcp)

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
