# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(suspendresumecontrol_auto LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

include_directories(../../qtwasmtestlib/)

qt_internal_add_manual_test(suspendresumecontrol_auto
    SOURCES
        main.cpp
        suspendresumecontrol_auto.html
        ../../qtwasmtestlib/qtwasmtestlib.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
)

add_custom_target(
    UpdateFile_suspendresumecontrol_auto ALL
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/suspendresumecontrol_auto.html
            ${CMAKE_CURRENT_BINARY_DIR}/suspendresumecontrol_auto.html
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
            ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/suspendresumecontrol_auto.html)

add_dependencies(UpdateFile_suspendresumecontrol_auto suspendresumecontrol_auto)

add_custom_command(
    TARGET suspendresumecontrol_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../qtwasmtestlib/qtwasmtestlib.js
            ${CMAKE_CURRENT_BINARY_DIR}/qtwasmtestlib.js)
