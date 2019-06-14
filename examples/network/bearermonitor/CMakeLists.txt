# Generated from bearermonitor.pro.

cmake_minimum_required(VERSION 3.14)
project(bearermonitor LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Network)
find_package(Qt6 COMPONENTS Widgets)

add_executable(bearermonitor
    bearermonitor.cpp bearermonitor.h
    bearermonitor_240_320.ui
    bearermonitor_640_480.ui
    main.cpp
    sessionwidget.cpp sessionwidget.h sessionwidget.ui
)
target_link_libraries(bearermonitor PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::Widgets
)

install(TARGETS bearermonitor
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
