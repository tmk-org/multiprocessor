cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "cmdsrv")

project(${PROJECT_NAME}
    DESCRIPTION "Network communication text-based command api"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/api"
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
include_directories(${CMAKE_SOURCE_DIR}/../tcp/include)
include_directories(${CMAKE_SOURCE_DIR}/../misc/include)
include_directories(${CMAKE_SOURCE_DIR}/../model/include)

add_library(${PROJECT_NAME} SHARED
    #headers
    include/command_server/command_server.h
    #sources
    src/command_server.c
)
target_link_libraries(${PROJECT_NAME} model misc)

set(${PROJECT_NAME}_PUBLIC_HEADERS
    include/command_server/command_server.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    PUBLIC_HEADER "{${PROJECT_NAME}_PUBLIC_HEADERS}"
)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(cclient examples/client.c)
target_link_libraries(cclient tcp)

add_executable(cserver examples/server.c)
target_link_libraries(cserver tcp ${PROJECT_NAME} -pthread)

###############################################################################
# Tests
###############################################################################

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

set(PROJECT_TESTS ${PROJECT_NAME}-tests)

#add_executable(${PROJECT_TESTS}
#    tests/ApiTests.cpp
#)

#add_executable(${PROJECT_TESTS}
#    tests/api_tests.c
#)

#target_link_libraries(${PROJECT_TESTS}
#    ${GTEST_BOTH_LIBRARIES} pthread
#    ${PROJECT_NAME}
#)

#add_test(${PROJECT_TESTS} ${PROJECT_TESTS})
#add_test(NAME ${PROJECT_TESTS} COMMAND $<TARGET_FILE:${PROJECT_TESTS}>)
