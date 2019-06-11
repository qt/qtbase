# Generated from fortuneclient.pro.

cmake_minimum_required(VERSION 3.14)
project(fortuneclient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Network)
find_package(Qt5 COMPONENTS Widgets)

add_qt_gui_executable(fortuneclient WIN32 MACOSX_BUNDLE
    client.cpp client.h
    main.cpp
)
target_link_libraries(fortuneclient PUBLIC
    Qt::Network
    Qt::Widgets
)

install(TARGETS fortuneclient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
