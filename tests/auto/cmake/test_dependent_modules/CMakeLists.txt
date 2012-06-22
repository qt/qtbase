
cmake_minimum_required(VERSION 2.8)

project(test_dependent_modules)

# The module finds its dependencies
find_package(Qt5Widgets REQUIRED)

include_directories(${Qt5Widgets_INCLUDE_DIRS})

add_definitions(${Qt5Core_DEFINITIONS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt5_wrap_cpp(moc_files mywidget.h)
qt5_wrap_ui(ui_files mywidget.ui)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

add_executable(mywidget mywidget.cpp ${moc_files} ${ui_files})
target_link_libraries(mywidget ${Qt5Widgets_LIBRARIES})
