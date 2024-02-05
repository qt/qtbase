# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_collecting_plugins
    VERSION
        "${CMAKE_Core_MODULE_MAJOR_VERSION}.${CMAKE_Core_MODULE_MINOR_VERSION}.${CMAKE_Core_MODULE_PATCH_VERSION}")

find_package(Qt6 COMPONENTS Core Gui BuildInternals REQUIRED)

qt_prepare_standalone_project()

qt_internal_add_plugin(QTestImagePlugin
    SHARED
    PLUGIN_TYPE imageformats
    SOURCES
        plugin.cpp
    LIBRARIES
        Qt6::Gui
    SKIP_INSTALL # Make sure that we do not package this plugin
)

qt_add_executable(TestExecutable main.cpp)
target_link_libraries(TestExecutable PRIVATE Qt6::Gui)

__qt_internal_collect_plugin_targets_from_dependencies(TestExecutable plugin_targets)

if(NOT "QTestImagePlugin" IN_LIST plugin_targets)
    message(FATAL_ERROR "QTestImagePlugin plugin is missing")
endif()
