cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "capture")

project(${PROJECT_NAME}
    DESCRIPTION "Opencv camera and client with opencv filter for this one"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/arvcamera"
    LANGUAGES C CXX)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -g")

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

add_library(aravis SHARED IMPORTED)
set_property(TARGET aravis PROPERTY
             IMPORTED_LOCATION /usr/lib/x86_64-linux-gnu/libaravis-0.8.so)
target_link_libraries(${TARGET} aravis)

###############################################################################
# Add sources
###############################################################################

#include(GenerateVersion)

add_library(${PROJECT_NAME} SHARED
    #header
    include/${PROJECT_NAME}/list.h
    include/${PROJECT_NAME}/ring_buffer.h
    #sources
    src/ring_buffer.c
)

target_link_libraries(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)

set(${PROJECT_NAME}_PUBLIC_HEADERS
    include/${PROJECT_NAME}/list.h
    include/${PROJECT_NAME}/ring_buffer.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER "{${PROJECT_NAME}_PUBLIC_HEADERS}"
)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(finddevices tools/finddevices.c)
target_link_libraries(finddevices ${PROJECT_NAME})

add_executable(udpcapture tools/capture.c)
target_link_libraries(udpcapture ${PROJECT_NAME} -pthread)

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
