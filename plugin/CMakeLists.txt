cmake_minimum_required(VERSION 3.16)
project(reaper_csurf_ipc VERSION 1.0 LANGUAGES CXX)

# Define the REAPER plugin directory
if(APPLE)
    set(REAPER_USER_PLUGINS_DIR "$ENV{HOME}/Library/Application Support/REAPER/UserPlugins")
else()
    set(REAPER_USER_PLUGINS_DIR "$ENV{HOME}/.config/REAPER/UserPlugins")
endif()

message(STATUS "REAPER UserPlugins directory set to: ${REAPER_USER_PLUGINS_DIR}")

# Create the library
add_library(reaper_csurf_ipc SHARED
    reaper_plugin_stub.cpp
)

# Set the output name
if(APPLE)
    set_target_properties(reaper_csurf_ipc PROPERTIES
        OUTPUT_NAME "reaper_csurf_ipc"
        SUFFIX ".dylib"
        PREFIX ""
    )
else()
    set_target_properties(reaper_csurf_ipc PROPERTIES
        OUTPUT_NAME "reaper_csurf_ipc"
        PREFIX ""
    )
endif()

# Find Threads
find_package(Threads REQUIRED)

# Link necessary libraries
target_link_libraries(reaper_csurf_ipc PRIVATE
    Threads::Threads
)

# Tell the compiler where to find the REAPER SDK headers
target_include_directories(reaper_csurf_ipc PRIVATE
    ${REAPER_SDK_DIR}
    ${REAPER_SDK_DIR}/../
)

# Set C++ standard
set_target_properties(reaper_csurf_ipc PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Install step
install(TARGETS reaper_csurf_ipc
    LIBRARY DESTINATION ${REAPER_USER_PLUGINS_DIR}
    COMPONENT "Plugins"
)