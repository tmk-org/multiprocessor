cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "filereader")

project(${PROJECT_NAME}
    DESCRIPTION "Light-weight examples for work with different sources"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/filereader"
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
    include/${PROJECT_NAME}/filelist.h
    include/${PROJECT_NAME}/imageprocess.h
    #sources
    src/filereader.c
    src/filelist.c
    src/imageprocess.c
)

set(${PROJECT_NAME}_PUBLIC_HEADERS
    include/${PROJECT_NAME}/filelist.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER "{${PROJECT_NAME}_PUBLIC_HEADERS}"
)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

#add_executable(freader src/filereader.c)
#target_link_libraries(freader ${PROJECT_NAME})

add_executable(freader src/ocvprocess.cpp src/filereader.c)
target_link_libraries(freader ${PROJECT_NAME} opencv_core opencv_highgui opencv_imgproc)

###############################################################################
# Tests
###############################################################################

#enable_testing()
#find_package(GTest REQUIRED)
#include_directories(${GTEST_INCLUDE_DIR})

#set(PROJECT_TESTS ${PROJECT_NAME}-tests)

#add_executable(${PROJECT_TESTS}
#    tests/FilterTests.cpp
#)

#add_executable(${PROJECT_TESTS}
#    tests/filter_tests.c
#)

#target_link_libraries(${PROJECT_TESTS}
#    ${GTEST_BOTH_LIBRARIES} pthread
#    opencv_core opencv_highgui
#    ${PROJECT_NAME}
#)

#add_test(${PROJECT_TESTS} ${PROJECT_TESTS})
#add_test(NAME ${PROJECT_TESTS} COMMAND $<TARGET_FILE:${PROJECT_TESTS}>)
