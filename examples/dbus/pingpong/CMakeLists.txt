# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(pingpong LANGUAGES CXX)

if (ANDROID)
    message(FATAL_ERROR "This project cannot be built on Android.")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core DBus)

qt_standard_project_setup()

qt_add_executable(ping
    ping.cpp
    ping-common.h
)

target_link_libraries(ping PRIVATE
    Qt6::Core
    Qt6::DBus
)

qt_add_executable(pong
    ping-common.h
    pong.cpp
)

target_link_libraries(pong PRIVATE
    Qt6::Core
    Qt6::DBus
)

install(TARGETS ping pong
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ping
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

qt_generate_deploy_app_script(
    TARGET pong
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
