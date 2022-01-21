cmake_minimum_required(VERSION 3.16)
project(pingpong LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/dbus/pingpong")

find_package(Qt6 REQUIRED COMPONENTS Core DBus)

qt_add_executable(ping
    ping.cpp
    ping-common.h
)

target_link_libraries(ping PUBLIC
    Qt6::Core
    Qt6::DBus
)

qt_add_executable(pong
    ping-common.h
    pong.cpp pong.h
)

target_link_libraries(pong PUBLIC
    Qt6::Core
    Qt6::DBus
)

install(TARGETS ping pong
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
