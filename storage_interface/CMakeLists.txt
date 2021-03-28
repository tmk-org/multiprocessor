cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "storage_interface")

project(${PROJECT_NAME}
    DESCRIPTION "Boost shared memory server and client with libstorage"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/storage"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

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

find_package(Glog REQUIRED)
find_package(Boost REQUIRED)
find_package(OpenCV REQUIRED)

###############################################################################
# Add sources
###############################################################################

#include(GenerateVersion)

include_directories(${CMAKE_SOURCE_DIR}/libstorage/include)

add_library(${PROJECT_NAME} SHARED
    #headers
    include/${PROJECT_NAME}/storage_interface.h
    #sources
    src/storage_interface.cpp
)

target_link_libraries(${PROJECT_NAME} storage opencv_core opencv_highgui -lrt -lpthread)

set(${PROJECT_NAME}_PUBLIC_HEADERS
    include/${PROJECT_NAME}/storage_inteface.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    PUBLIC_HEADER "{${PROJECT_NAME}_PUBLIC_HEADERS}"
)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(sname examples/name.c)
target_link_libraries(sname ${PROJECT_NAME})

add_executable(sserver examples/storage_server.c)
target_link_libraries(sserver ${PROJECT_NAME} storage)

add_executable(storage_server examples/storage_server.cpp)
#target_link_libraries(storage_server opencv_core opencv_highgui storage rt pthread)
target_link_libraries(storage_server -lrt storage)

add_executable(storage_client_writer examples/storage_client_writer.cpp)
target_link_libraries(storage_client_writer storage)

add_executable(storage_client_reader examples/storage_client_reader.cpp)
target_link_libraries(storage_client_reader storage)

###############################################################################
# Tests
###############################################################################

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

set(PROJECT_TESTS ${PROJECT_NAME}-tests)

add_executable(${PROJECT_TESTS}
    tests/StorageInterfaceTests.cpp
)

#add_executable(${PROJECT_TESTS}
#    tests/storage_interface_tests.c
#)

target_link_libraries(${PROJECT_TESTS}
    ${GTEST_BOTH_LIBRARIES}
    pthread storage
    ${PROJECT_NAME}
)

#target_compile_definitions(${LIBRARY_TESTS} PRIVATE TEST_LOCAL_DATA_DIR="${TEST_LOCAL_DATA_DIR}"  ROOT_DIR="${ROOT_DIR}")

add_test(NAME ${PROJECT_TESTS} COMMAND $<TARGET_FILE:${PROJECT_TESTS}>)



