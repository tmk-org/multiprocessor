cmake_minimum_required(VERSION 3.16)

set(LIBRARY_NAME "dbpg")
project(${LIBRARY_NAME} VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

find_package(PQXX REQUIRED)

#add_library(${LIBRARY_NAME}
#)

#target_include_directories(${LIBRARY_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
#target_link_libraries(${LIBRARY_NAME} ${PQXX_LIB}
#)

add_executable(pgconnect examples/connect.cpp)
target_link_libraries(pgconnect pqxx)

#### Tests ####
# Setup testing
#enable_testing()
#find_package(GTest REQUIRED)
#include_directories(${GTEST_INCLUDE_DIR})

#set(LIBRARY_TESTS ${LIBRARY_NAME}-tests)
#add_executable(${LIBRARY_TESTS}
#)
#target_link_libraries(${LIBRARY_TESTS} ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARIES} ${LIBRARY_NAME})
#target_compile_definitions(${LIBRARY_TESTS} PRIVATE TEST_LOCAL_DATA_DIR="${TEST_LOCAL_DATA_DIR}"  ROOT_DIR="${ROOT_DIR}")
#add_test(${LIBRARY_TESTS} ${LIBRARY_TESTS})
