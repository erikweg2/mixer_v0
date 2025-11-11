#
# TOP-LEVEL CMAKE (reaper_qt_project/CMakeLists.txt)
#

cmake_minimum_required(VERSION 3.16)
project(mixer_v0 VERSION 1.0 LANGUAGES C CXX)

# --- Find Qt6
# This tells CMake to find the Qt6 package.
# It will look in standard system paths.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 COMPONENTS Widgets Network REQUIRED)

# --- Find pthreads (for the plugin and hub)
find_package(Threads REQUIRED)

# --- REAPER SDK Path
# This creates a variable REAPER_SDK_DIR that the user MUST pass
# on the command line, e.g.: cmake .. -DREAPER_SDK_DIR=/path/to/sdk
message(STATUS "Looking for REAPER SDK...")
if(NOT DEFINED REAPER_SDK_DIR)
    message(FATAL_ERROR "REAPER_SDK_DIR is not set. Please define it, e.g.: -DREAPER_SDK_DIR=/path/to/reaper_sdk")
endif()
message(STATUS "REAPER SDK found at: ${REAPER_SDK_DIR}")

# --- Add oscpack as a subdirectory
# This tells CMake to build oscpack from our 'libs' folder
add_subdirectory(libs/oscpack)

# --- FIX for oscpack ---
# Add the oscpack root directory to its own include path
target_include_directories(oscpack PUBLIC ${CMAKE_SOURCE_DIR}/libs/oscpack)
# --- End FIX ---

# --- Add our three components
# This tells CMake to look in the 'plugin', 'hub', and 'gui'
# folders and process their own CMakeLists.txt files.
add_subdirectory(plugin)
add_subdirectory(hub)
add_subdirectory(gui)
