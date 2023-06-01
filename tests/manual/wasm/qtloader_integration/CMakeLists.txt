# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(tst_qtloader_integration
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)

set_target_properties(tst_qtloader_integration PROPERTIES QT_WASM_EXTRA_EXPORTED_METHODS "ENV")

add_custom_command(
    TARGET tst_qtloader_integration POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/tst_qtloader_integration.html
            ${CMAKE_CURRENT_BINARY_DIR}/tst_qtloader_integration.html)

add_custom_command(
    TARGET tst_qtloader_integration POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/plugins/platforms/wasm/qtloader.js
            ${CMAKE_CURRENT_BINARY_DIR}/qtloader.js)

add_custom_command(
    TARGET tst_qtloader_integration POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../shared/testrunner.js
            ${CMAKE_CURRENT_BINARY_DIR}/testrunner.js)

add_custom_command(
    TARGET tst_qtloader_integration POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/test_body.js
            ${CMAKE_CURRENT_BINARY_DIR}/test_body.js)

add_custom_command(
    TARGET tst_qtloader_integration POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/preload.json
            ${CMAKE_CURRENT_BINARY_DIR}/preload.json)
