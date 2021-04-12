cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME "config")

project(${PROJECT_NAME}
    DESCRIPTION "Json configuration file parser"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor/config"
    LANGUAGES C CXX
)

add_library(${PROJECT_NAME} SHARED
    #headers
    include/${PROJECT_NAME}/config.h
    ../model/include/model/module_t.h
    #sources
    src/config.cpp
)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# TODO replace target_include_directories with include_directories and target_link_libraries

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../misc/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories (${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../model/include>
    $<INSTALL_INTERFACE:include>
)

#target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)

# Test
enable_testing()
#find_package(GTest REQUIRED)
#include_directories(${GTEST_INCLUDE_DIR})
add_executable(config_test tests/config_test.cpp)
target_link_libraries(config_test gtest pthread gtest gtest_main) #${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARIES}
target_link_libraries(config_test ${PROJECT_NAME})
#add_test(config_test config_test)
