cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "cvcamera")

project(${PROJECT_NAME}
    DESCRIPTION "Opencv camera and client with opencv filter for this one"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/camera"
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
    include/${PROJECT_NAME}/cvcamera.h
    include/${PROJECT_NAME}/cvcollector.h
    include/${PROJECT_NAME}/cvgenerator.h
    #sources
    src/cvcollector.cpp
    src/cvgenerator.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/cvfilter/include)
target_link_libraries(${PROJECT_NAME} opencv_core opencv_highgui opencv_imgproc -pthread cvfilter)

set(${PROJECT_NAME}_PUBLIC_HEADERS
    include/${PROJECT_NAME}/cvcamera.h
    include/${PROJECT_NAME}/cvcollector.h
    include/${PROJECT_NAME}/cvgenerator.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER "{${PROJECT_NAME}_PUBLIC_HEADERS}"
)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

#add_executable(cvtest examples/cvtest.cpp)
#target_link_libraries(cvtest opencv_core opencv_highgui)

#add_executable(cvdata examples/cvdata.cpp)
#target_link_libraries(cvdata opencv_core opencv_highgui)

add_executable(cvserver tools/cvserver.cpp)
target_link_libraries(cvserver -pthread ${PROJECT_NAME})

add_executable(cvclient tools/cvclient.cpp)
target_link_libraries(cvclient -pthread ${PROJECT_NAME})

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
