# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qwasmwindow_harness LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qwasmwindow_harness
    MANUAL
    NO_BATCH
    NO_WASM_DEFAULT_FILES
    SOURCES
        tst_qwasmwindow_harness.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::OpenGL
        Qt::Widgets
)

# Resources:
set(shaders_resource_files
    "fshader.glsl"
    "vshader.glsl"
)

qt6_add_resources(tst_qwasmwindow_harness "shaders"
    PREFIX
        "/"
    FILES
        ${shaders_resource_files}
)

if(CMAKE_HOST_WIN32)
    SET(RUNSHCMD run.bat)
    SET(RUNSHARG "NotUsed")
else()
    SET(RUNSHCMD bash)
    SET(RUNSHARG run.sh)
endif()

set_target_properties(tst_qwasmwindow_harness PROPERTIES CROSSCOMPILING_EMULATOR "") # disabling emrun
qt_internal_create_test_script(NAME tst_qwasmwindow_harness
    COMMAND ${RUNSHCMD}
    ARGS    ${RUNSHARG}
    WORKING_DIRECTORY "${test_working_dir}"
    OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/tst_qwasmwindow_harnessWrapper$<CONFIG>.cmake"
)

add_custom_command(
    TARGET tst_qwasmwindow_harness POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/tst_qwasmwindow_harness_run.html
            ${CMAKE_CURRENT_BINARY_DIR}/tst_qwasmwindow_harness_run.html
)

add_custom_command(
    TARGET tst_qwasmwindow_harness POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../util/wasm/qtwasmserver/qtwasmserver.py
            ${CMAKE_CURRENT_BINARY_DIR}/qtwasmserver.py
)

add_custom_command(
    TARGET tst_qwasmwindow_harness POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/qwasmwindow.py
            ${CMAKE_CURRENT_BINARY_DIR}/qwasmwindow.py
)

add_custom_command(
    TARGET tst_qwasmwindow_harness POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/run.sh
            ${CMAKE_CURRENT_BINARY_DIR}/run.sh
)

add_custom_command(
    TARGET tst_qwasmwindow_harness POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/run.bat
            ${CMAKE_CURRENT_BINARY_DIR}/run.bat
)
