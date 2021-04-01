cmake_minimum_required(VERSION 3.11)

set(PROJECT_NAME "multiprocessor")

project(${PROJECT_NAME}
    DESCRIPTION "Multiprocess streamer with shared memory and command server"
    HOMEPAGE_URL "https://github.com/kat-spb/multiprocessor"
    LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${${PROJECT_NAME}_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${${PROJECT_NAME}_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${${PROJECT_NAME}_BINARY_DIR}/lib)
file(MAKE_DIRECTORY ${${PROJECT_NAME}_BINARY_DIR}/include)

###############################################################################
# Include CMake dependencies
###############################################################################

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

###############################################################################
# Options
###############################################################################

#option(PIPELINE "Whether run in pipeline mode" OFF)
#option(STREAMER "Whether run in streamer mode" ON)

###############################################################################
# Find packages
###############################################################################

###############################################################################
# Compiler specific configuration
###############################################################################

if(CMAKE_BUILD_TYPE)
    message(STATUS "Build type specified as ${CMAKE_BUILD_TYPE}")
else()
    message(STATUS "Build type not specified, using Release with debug information")
    set(CMAKE_BUILD_TYPE RelWithDeb)
    set(IS_DEBUG FALSE)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

###############################################################################
# CCACHE if it possible
###############################################################################

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

###############################################################################
# Add sources
###############################################################################

add_subdirectory(misc)
add_subdirectory(api)
add_subdirectory(tcp)
add_subdirectory(config)
add_subdirectory(pmngr)
add_subdirectory(shmem)
add_subdirectory(streamer)
add_subdirectory(model)
add_subdirectory(cvfilter)
add_subdirectory(cvcamera)
add_subdirectory(libstorage)
add_subdirectory(storage_interface)

###############################################################################
# Tests
###############################################################################

#file(WRITE "${CMAKE_BINARY_DIR}/CTestCustom.cmake"
#    "set(CTEST_CUSTOM_PRE_TEST ${CMAKE_MAKE_PROGRAM})\n"
#)

include(CTest)

###############################################################################
# Install/uninstall script
###############################################################################

install(DIRECTORY "${CMAKE_SOURCE_DIR}/" # source directory
        DESTINATION "include" # target directory
        FILES_MATCHING # install only matched files
        PATTERN "*.h" # select header files
)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE STRING "" FORCE)
endif()

#configure_file("${CMAKE_SOURCE_DIR}/cmake/CMakeUninstall.cmake"
#               "${CMAKE_CURRENT_BINARY_DIR}/CMakeUninstall.cmake"
#               IMMEDIATE @ONLY)
#add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR
#set_target_properties(uninstall PROPERTIES FOLDER ${CMAKE_TARGETS_ROOT_FOLDER})
