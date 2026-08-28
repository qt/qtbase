# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(sockify_sockets_auto LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(sockify_sockets_auto
    SOURCES
        main.cpp
        ../../qtwasmtestlib/qtwasmtestlib.cpp
    LIBRARIES
        Qt::Core
        Qt::Network
)

include_directories(../../qtwasmtestlib/)

add_custom_command(
    TARGET sockify_sockets_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/sockify_sockets_auto.html
            ${CMAKE_CURRENT_BINARY_DIR}/sockify_sockets_auto.html)

add_custom_command(
    TARGET sockify_sockets_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../qtwasmtestlib/qtwasmtestlib.js
            ${CMAKE_CURRENT_BINARY_DIR}/qtwasmtestlib.js)
