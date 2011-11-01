
cmake_minimum_required(VERSION 2.8)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)

include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt5_wrap_cpp(moc_files mywidget.h)
qt5_wrap_ui(ui_files mywidget.ui)

add_executable(mywidget mywidget.cpp ${moc_files} ${ui_files})
target_link_libraries(mywidget ${Qt5Core_LIBRARY} ${Qt5Gui_LIBRARY} ${Qt5Widgets_LIBRARY})
