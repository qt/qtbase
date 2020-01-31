# Generated from echowindow.pro.

cmake_minimum_required(VERSION 3.14)
project(echoplugin LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(echopluginwindow # special case: renamed target
    echointerface.h
    echowindow.cpp echowindow.h
    main.cpp
)
target_link_libraries(echopluginwindow PUBLIC # special case: renamed target
    Qt::Widgets
)

install(TARGETS echopluginwindow # special case: renamed target
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
