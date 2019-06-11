# Generated from broadcastsender.pro.

cmake_minimum_required(VERSION 3.14)
project(broadcastsender LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Network)
find_package(Qt5 COMPONENTS Widgets)

add_qt_gui_executable(broadcastsender WIN32 MACOSX_BUNDLE
    main.cpp
    sender.cpp sender.h
)
target_link_libraries(broadcastsender PUBLIC
    Qt::Network
    Qt::Widgets
)

install(TARGETS broadcastsender
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
