 #
# HUB CMAKE (reaper_qt_project/hub/CMakeLists.txt)
#

# Create the executable
add_executable(hub_app
    hub_app_stub.cpp
)

# Link necessary libraries
target_link_libraries(hub_app PRIVATE
    Threads::Threads
    oscpack # This is the library we built from libs/oscpack
)

# Tell the compiler where to find the oscpack headers
target_include_directories(hub_app PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/oscpack
)
