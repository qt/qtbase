# Generated from pong.pro.

cmake_minimum_required(VERSION 3.14)
project(pong LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS DBus)

add_qt_gui_executable(pong
    ping-common.h
    pong.cpp pong.h
)
target_link_libraries(pong PUBLIC
    # Remove: gui
    Qt::DBus
)
install(TARGETS pong
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
