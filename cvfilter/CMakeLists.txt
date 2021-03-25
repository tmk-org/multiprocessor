cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "cvfilter")

project(${PROJECT_NAME}
    DESCRIPTION "Opencv filter for found object"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/cvfilter"
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

find_package(OpenCV REQUIRED)

###############################################################################
# Add sources
###############################################################################

#include(GenerateVersion)

add_library(${PROJECT_NAME} SHARED
    #headers
    include/cvfilter/cvfilter.h
    include/cvfilter/cvparams.h
    #sources
    src/cvfilter.cpp
)

target_link_libraries(${PROJECT_NAME} opencv_core opencv_highgui opencv_imgproc)

set(${PROJECT_NAME}_PUBLIC_HEADERS
    include/${PROJECT_NAME}/cvfilter.h
    include/${PROJECT_NAME}/cvparams.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    PUBLIC_HEADER "{${PROJECT_NAME}_PUBLIC_HEADERS}"
)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(cvradius examples/cvradius.cpp)
target_link_libraries(cvradius opencv_core opencv_highgui opencv_imgproc ${PROJECT_NAME})

###############################################################################
# Tests
###############################################################################

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

set(PROJECT_TESTS ${PROJECT_NAME}-tests)

add_executable(${PROJECT_TESTS}
    tests/FilterTests.cpp
)

#add_executable(${PROJECT_TESTS}
#    tests/filter_tests.c
#)

target_link_libraries(${PROJECT_TESTS}
    ${GTEST_BOTH_LIBRARIES} pthread
    opencv_core opencv_highgui
    ${PROJECT_NAME}
)

#add_test(${PROJECT_TESTS} ${PROJECT_TESTS})
add_test(NAME ${PROJECT_TESTS} COMMAND $<TARGET_FILE:${PROJECT_TESTS}>)
