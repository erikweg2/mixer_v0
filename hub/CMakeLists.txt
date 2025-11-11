cmake_minimum_required(VERSION 3.16)
project(hub_app VERSION 1.0 LANGUAGES CXX)

# Create the executable
add_executable(hub_app
    hub_app_stub.cpp
)

# Find Threads
find_package(Threads REQUIRED)

# Link necessary libraries
target_link_libraries(hub_app PRIVATE
    Threads::Threads
    oscpack
)

# Tell the compiler where to find the oscpack headers
target_include_directories(hub_app PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/oscpack
)

# Set C++ standard
set_target_properties(hub_app PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)