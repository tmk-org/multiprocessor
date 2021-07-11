cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "arvcamera")

project(${PROJECT_NAME}
    DESCRIPTION "Opencv camera and client with opencv filter for this one"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/arvcamera"
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

add_library(aravis SHARED IMPORTED)
set_property(TARGET aravis PROPERTY
             IMPORTED_LOCATION /usr/lib/x86_64-linux-gnu/libaravis-0.8.so)
target_link_libraries(${TARGET} aravis)

###############################################################################
# Add sources
###############################################################################

#include(GenerateVersion)

add_library(${PROJECT_NAME} SHARED
    #headers
    #include/${PROJECT_NAME}/arvcamera.h
    include/${PROJECT_NAME}/arvcollector.h
    include/${PROJECT_NAME}/arvgenerator.h
    #sources
    src/arvcollector.cpp
    src/arvgenerator.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC "/usr/include/aravis-0.8")
target_include_directories(${PROJECT_NAME} PUBLIC "/usr/include/glib-2.0/")
target_include_directories(${PROJECT_NAME} PUBLIC "/usr/lib/x86_64-linux-gnu/glib-2.0/include")

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/cvfilter/include)
target_link_libraries(${PROJECT_NAME} aravis glib-2.0 gobject-2.0 opencv_core opencv_highgui cvfilter)

set(${PROJECT_NAME}_PUBLIC_HEADERS
    #include/${PROJECT_NAME}/arvcamera.h
    include/${PROJECT_NAME}/arvcollector.h
    include/${PROJECT_NAME}/arvgenerator.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER "{${PROJECT_NAME}_PUBLIC_HEADERS}"
)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(arvsource examples/arvsource.cpp)
target_include_directories(arvsource PUBLIC "/usr/include/aravis-0.8")
target_include_directories(arvsource PUBLIC "/usr/include/glib-2.0/")
target_include_directories(arvsource PUBLIC "/usr/lib/x86_64-linux-gnu/glib-2.0/include")
target_link_libraries(arvsource -pthread aravis)

add_executable(arvcapture examples/arvcapture.cpp)
target_include_directories(arvcapture PUBLIC "/usr/include/aravis-0.8")
target_include_directories(arvcapture PUBLIC "/usr/include/glib-2.0/")
target_include_directories(arvcapture PUBLIC "/usr/lib/x86_64-linux-gnu/glib-2.0/include")
target_link_libraries(arvcapture -pthread aravis glib-2.0 gobject-2.0 opencv_core opencv_highgui)

add_executable(arvserver tools/arvserver.cpp)
target_link_libraries(arvserver ${PROJECT_NAME})

add_executable(arvclient tools/arvclient.cpp)
target_link_libraries(arvclient ${PROJECT_NAME})
target_include_directories(arvclient PUBLIC "/usr/include/aravis-0.8")
target_include_directories(arvclient PUBLIC "/usr/include/glib-2.0/")
target_include_directories(arvclient PUBLIC "/usr/lib/x86_64-linux-gnu/glib-2.0/include")
target_link_libraries(arvclient -pthread aravis glib-2.0 gobject-2.0 opencv_core opencv_highgui)

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
