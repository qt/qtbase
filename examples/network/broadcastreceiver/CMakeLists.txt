# Generated from broadcastreceiver.pro.

cmake_minimum_required(VERSION 3.14)
project(broadcastreceiver LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Network)
find_package(Qt5 COMPONENTS Widgets)

add_qt_gui_executable(broadcastreceiver WIN32 MACOSX_BUNDLE
    main.cpp
    receiver.cpp receiver.h
)
target_link_libraries(broadcastreceiver PUBLIC
    Qt::Network
    Qt::Widgets
)

install(TARGETS broadcastreceiver
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
