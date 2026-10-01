# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(test_plugin_deployment_exclude_plugin_types)
enable_testing()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Test)

qt6_standard_project_setup()

include(../test_plugin_deployment_common/test_plugin_deployment_common.cmake)

# No image format plugins must be deployed.
create_test_executable(App
    DEPLOY_OPTIONS EXCLUDE_PLUGIN_TYPES imageformats
)
