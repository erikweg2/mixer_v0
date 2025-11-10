#
# PLUGIN CMAKE (reaper_qt_project/plugin/CMakeLists.txt)
#

# Define the REAPER plugin directory
set(REAPER_USER_PLUGINS_DIR "$ENV{HOME}/.config/REAPER/UserPlugins")
message(STATUS "REAPER UserPlugins directory set to: ${REAPER_USER_PLUGINS_DIR}")

# Create the library (a .so file)
add_library(reaper_csurf_ipc SHARED
    reaper_plugin_stub.cpp
)

# Set the output name
set_target_properties(reaper_csurf_ipc PROPERTIES
    OUTPUT_NAME "reaper_csurf_ipc"
    PREFIX "" # Removes the 'lib' prefix
)

# Link necessary libraries
target_link_libraries(reaper_csurf_ipc PRIVATE
    Threads::Threads
)

# Tell the compiler where to find the REAPER SDK headers
target_include_directories(reaper_csurf_ipc PRIVATE
    ${REAPER_SDK_DIR}

    # --- FINAL, STANDARD FIX ---
    # The 'sdk' dir is at ${REAPER_SDK_DIR}
    # We moved 'WDL' to be next to it, so we add
    # the parent directory:
    ${REAPER_SDK_DIR}/../
    # This allows the compiler to find "../WDL/swell/swell.h"
    # --- END FINAL FIX ---
)

# --- Install Step ---
# This creates a 'make install' command to copy the .so
# file to the correct REAPER directory.
install(TARGETS reaper_csurf_ipc
    LIBRARY DESTINATION ${REAPER_USER_PLUGINS_DIR}
    COMPONENT "Plugins"
)
