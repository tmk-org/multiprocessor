cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "api")

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

add_library(${PROJECT_NAME} SHARED
    #headers
    include/api/api.h
    include/api/api_parser.h
    #sources
    src/api.c
    src/api_parser.c
)

set(${PROJECT_NAME}_PUBLIC_HEADERS
    include/api/api.h
    include/api/api_parser.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    PUBLIC_HEADER "{${PROJECT_NAME}_PUBLIC_HEADERS}"
)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(api_example examples/example.c)
target_link_libraries(api_example api)

###############################################################################
# Tests
###############################################################################

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

set(PROJECT_TESTS ${PROJECT_NAME}-tests)

add_executable(${PROJECT_TESTS}
    tests/ApiTests.cpp
)

#add_executable(${PROJECT_TESTS}
#    tests/api_tests.c
#)

target_link_libraries(${PROJECT_TESTS}
    ${GTEST_BOTH_LIBRARIES} pthread
    ${PROJECT_NAME}
)

#add_test(${PROJECT_TESTS} ${PROJECT_TESTS})
add_test(NAME ${PROJECT_TESTS} COMMAND $<TARGET_FILE:${PROJECT_TESTS}>)
