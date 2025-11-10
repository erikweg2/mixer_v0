 
#
# GUI CMAKE (reaper_qt_project/gui/CMakeLists.txt)
#

# Create the executable
add_executable(qt_gui_app
    qt_gui_stub.cpp
)

# Link necessary libraries
# CMake handles all the Qt linking automatically
target_link_libraries(qt_gui_app PRIVATE
    Qt6::Widgets
    Qt6::Network
    oscpack # This is the library we built from libs/oscpack
)

# Tell the compiler where to find the oscpack headers
target_include_directories(qt_gui_app PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/oscpack
)
