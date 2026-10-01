# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(test_plugin_deployment_lib)
enable_testing()

find_package(Qt6 REQUIRED COMPONENTS Core Test)
if(NOT UNIX OR APPLE)
    find_package(Qt6 REQUIRED COMPONENTS Gui)
endif()

qt6_standard_project_setup()
add_subdirectory(mylib)

include(../test_plugin_deployment_common/test_plugin_deployment_common.cmake)

# The app reaches Gui, and its plugins, only through MyLib.
create_test_executable(App LIBRARY MyLib)
