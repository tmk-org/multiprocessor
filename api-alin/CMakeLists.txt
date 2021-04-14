cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "api-alin")

project(${PROJECT_NAME}
    DESCRIPTION "Network communication text-based command api -.-"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/api-alin"
    LANGUAGES C CXX)

###############################################################################
# Include CMake dependencies
###############################################################################

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

###############################################################################
# Options
###############################################################################

#set(CMAKE_CXX_FLAGS "-g -Wall -Wextra -Werror")
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

###############################################################################
# Find packages
###############################################################################

###############################################################################
# Add sources
###############################################################################

#include(GenerateVersion)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(${PROJECT_NAME} SHARED
    #headers
    include/api/api.inl
    include/api/api.h
#   include/api/api_parser.h
    #sources
    src/api.c
#   src/api_parser.c
)

set(${PROJECT_NAME}_PUBLIC_HEADERS
#   include/api/api.inl
    include/api/api.h
#    include/api/api_parser.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    PUBLIC_HEADER "{${PROJECT_NAME}_PUBLIC_HEADERS}"
)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

#add_executable(api_example examples/example.c)
#target_link_libraries(api_example api)

###############################################################################
# Tests
###############################################################################

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

add_executable(api-alin_test tests/api-alin_test.cpp)
target_link_libraries(api-alin_test gtest pthread gtest gtest_main)
#target_link_libraries(api-alin_test api)
link_directories(api-alin_test ./include)
add_test(NAME api-alin-test COMMAND $<TARGET_FILE:api-alin_test>)
