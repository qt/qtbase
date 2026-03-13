# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
include_directories(../../qtwasmtestlib/)

qt_internal_add_manual_test(eventloop_auto
    SOURCES
        eventloop_auto.html
        main.cpp
        ../../qtwasmtestlib/qtwasmtestlib.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
)

add_custom_target(
    UpdateFile_eventloop_auto ALL
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/eventloop_auto.html
            ${CMAKE_CURRENT_BINARY_DIR}/eventloop_auto.html
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/eventloop_auto.html
            ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)

add_dependencies(UpdateFile_eventloop_auto eventloop_auto)

add_custom_command(
    TARGET eventloop_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../qtwasmtestlib/qtwasmtestlib.js
            ${CMAKE_CURRENT_BINARY_DIR}/qtwasmtestlib.js)
