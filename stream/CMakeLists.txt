cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "stream")

project(${PROJECT_NAME}
    DESCRIPTION "Opencv camera and client on aravis with opencv filter for this one"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/stream"
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

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED gio-2.0)
#target_include_directories(mytarget PRIVATE ${GLIB_INCLUDE_DIRS})
#target_link_libraries(mytarget INTERFACE ${GLIB_LDFLAGS})

###############################################################################
# Add sources
###############################################################################

#include(GenerateVersion)

#add_library(${PROJECT_NAME} SHARED
    #headers
#    include/${PROJECT_NAME}/params.h
#    include/${PROJECT_NAME}/collector.h
#    include/${PROJECT_NAME}/generator.h
    #sources
#    src/collector.c
#    src/generator.c
#)

#target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/cvfilter/include)
#target_link_libraries(${PROJECT_NAME} opencv_core opencv_highgui opencv_imgproc -pthread cvfilter)

#set(${PROJECT_NAME}_PUBLIC_HEADERS
#    include/${PROJECT_NAME}/cvcamera.h
#    include/${PROJECT_NAME}/cvcollector.h
#    include/${PROJECT_NAME}/cvgenerator.h
#)

#set_target_properties(${PROJECT_NAME} PROPERTIES
#    PUBLIC_HEADER "{${PROJECT_NAME}_PUBLIC_HEADERS}"
#)

#target_include_directories (${PROJECT_NAME} PUBLIC
#    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#    $<INSTALL_INTERFACE:include>
#)

add_executable(gserver examples/gserver.c)
target_include_directories(gserver PRIVATE ${GLIB_INCLUDE_DIRS})
#target_link_libraries(gserver INTERFACE ${GLIB_LDFLAGS})
target_link_libraries(gserver -pthread gio-2.0 gobject-2.0 glib-2.0)

add_executable(gclient examples/gclient.c)
target_include_directories(gclient PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(gclient -pthread gio-2.0 gobject-2.0 glib-2.0)

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
