# Generated from plugin.pro.

cmake_minimum_required(VERSION 3.14)
project(echoplugin LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/widgets/tools/echoplugin/plugins")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

add_library(echoplugin MODULE
    echoplugin.cpp echoplugin.h
)
target_include_directories(echoplugin PUBLIC
    ../echowindow
)

target_link_libraries(echoplugin PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS echoplugin
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
