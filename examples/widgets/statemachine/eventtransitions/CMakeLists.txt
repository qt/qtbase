# Generated from eventtransitions.pro.

cmake_minimum_required(VERSION 3.14)
project(eventtransitions LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/widgets/statemachine/eventtransitions")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(eventtransitions
    main.cpp
)
target_link_libraries(eventtransitions PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS eventtransitions
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
