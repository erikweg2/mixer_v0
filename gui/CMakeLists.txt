cmake_minimum_required(VERSION 3.16)
project(qt_gui_app VERSION 1.0 LANGUAGES CXX)

# Find Qt6 package
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# Create the executable
add_executable(qt_gui_app
    qt_gui_stub.cpp
)

# Link necessary libraries
target_link_libraries(qt_gui_app PRIVATE
    Qt6::Widgets
    Qt6::Network
    oscpack  # This now works because we added_subdirectory(oscpack)
)

# Tell the compiler where to find the oscpack headers
target_include_directories(qt_gui_app PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/oscpack
)

# Set C++ standard
set_target_properties(qt_gui_app PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)