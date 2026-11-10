cmake_minimum_required(VERSION 3.22)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../toolchain.json")
file(READ "${CMAKE_CURRENT_LIST_DIR}/../toolchain.json" monitor_toolchain)
string(JSON monitor_idf_version GET "${monitor_toolchain}" esp_idf)
string(JSON monitor_target GET "${monitor_toolchain}" target)

if(NOT DEFINED ENV{IDF_PATH} OR "$ENV{IDF_PATH}" STREQUAL "")
    message(FATAL_ERROR "Activate ESP-IDF ${monitor_idf_version} before configuring firmware.")
endif()

include("$ENV{IDF_PATH}/tools/cmake/version.cmake")
set(monitor_actual_idf "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}.${IDF_VERSION_PATCH}")
if(NOT monitor_actual_idf STREQUAL monitor_idf_version)
    message(FATAL_ERROR "Use ESP-IDF ${monitor_idf_version}; found ${monitor_actual_idf}.")
endif()

set(IDF_TARGET "${monitor_target}" CACHE STRING "Defender monitor MCU target")
if(NOT IDF_TARGET STREQUAL monitor_target)
    message(FATAL_ERROR "Use target ${monitor_target}; found ${IDF_TARGET}.")
endif()

# Compile application components and their declared, locked dependencies.
set(COMPONENTS main)
set(PROJECT_VER "0.0.0")
include("$ENV{IDF_PATH}/tools/cmake/project.cmake")
project(defender_monitor)
