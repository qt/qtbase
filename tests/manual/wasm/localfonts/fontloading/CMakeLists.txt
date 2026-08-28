# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(fontloading LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(fontloading
    GUI
    SOURCES
        main.cpp
        fontloading.html
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

add_custom_target(
    UpdateFile_fontloading ALL
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/fontloading.html
            ${CMAKE_CURRENT_BINARY_DIR}/fontloading.html
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
            ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/fontloading.html
)

add_dependencies(UpdateFile_fontloading fontloading)
