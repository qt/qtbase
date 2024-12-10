# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
qt_internal_add_manual_test(promise_auto
    SOURCES
        promise_main.cpp
        ../qtwasmtestlib/qtwasmtestlib.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
)

include_directories(../qtwasmtestlib/)

add_custom_command(
    TARGET promise_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/promise_auto.html
            ${CMAKE_CURRENT_BINARY_DIR}/promise_auto.html)

add_custom_command(
    TARGET promise_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../qtwasmtestlib/qtwasmtestlib.js
            ${CMAKE_CURRENT_BINARY_DIR}/qtwasmtestlib.js)

qt_internal_add_manual_test(files_auto
    SOURCES
        files_main.cpp
        ../qtwasmtestlib/qtwasmtestlib.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::GuiPrivate
)

include_directories(../qtwasmtestlib/)

add_custom_command(
    TARGET files_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/files_auto.html
            ${CMAKE_CURRENT_BINARY_DIR}/files_auto.html)

add_custom_command(
    TARGET files_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../qtwasmtestlib/qtwasmtestlib.js
            ${CMAKE_CURRENT_BINARY_DIR}/qtwasmtestlib.js)

qt_internal_add_manual_test(qwasmcompositor_auto
    SOURCES
        qwasmcompositor_main.cpp
        ../qtwasmtestlib/qtwasmtestlib.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::GuiPrivate
)

include_directories(../qtwasmtestlib/)

add_custom_command(
    TARGET qwasmcompositor_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/qwasmcompositor_auto.html
            ${CMAKE_CURRENT_BINARY_DIR}/qwasmcompositor_auto.html)

add_custom_command(
    TARGET qwasmcompositor_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../qtwasmtestlib/qtwasmtestlib.js
            ${CMAKE_CURRENT_BINARY_DIR}/qtwasmtestlib.js)

qt_internal_add_manual_test(iodevices_auto
    SOURCES
        iodevices_main.cpp
        ../qtwasmtestlib/qtwasmtestlib.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::GuiPrivate
)

add_custom_command(
    TARGET iodevices_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/iodevices_auto.html
            ${CMAKE_CURRENT_BINARY_DIR}/iodevices_auto.html)

add_custom_command(
    TARGET iodevices_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../qtwasmtestlib/qtwasmtestlib.js
            ${CMAKE_CURRENT_BINARY_DIR}/qtwasmtestlib.js)
