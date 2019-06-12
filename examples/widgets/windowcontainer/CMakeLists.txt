# Generated from windowcontainer.pro.

cmake_minimum_required(VERSION 3.14)
project(windowcontainer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_qt_gui_executable(windowcontainer
    ../../gui/openglwindow/openglwindow.cpp ../../gui/openglwindow/openglwindow.h
    windowcontainer.cpp
)
target_include_directories(windowcontainer PUBLIC
    ../../gui/openglwindow
)
target_link_libraries(windowcontainer PUBLIC
    Qt::Widgets
)

install(TARGETS windowcontainer
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
