# Generated from pingpong.pro.

cmake_minimum_required(VERSION 3.14)
project(ping LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/dbus/pingpong")

find_package(Qt6 COMPONENTS DBus)

qt_add_executable(ping
    ping.cpp
    ping-common.h
)
target_link_libraries(ping PUBLIC
    # Remove: gui
    Qt::DBus
)

install(TARGETS ping
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
cmake_minimum_required(VERSION 3.14)
project(pong LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/dbus/pingpong")

find_package(Qt6 COMPONENTS DBus)

qt_add_executable(pong
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
