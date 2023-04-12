# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)

project(TestStaticResources
        DESCRIPTION "Test of the static resources"
        HOMEPAGE_URL "https://qt.io/"
        LANGUAGES CXX C
        VERSION "${QT_REPO_MODULE_VERSION}"
)

find_package(Qt6 COMPONENTS Core BuildInternals Test CONFIG REQUIRED)
qt_internal_project_setup()

qt_build_repo_begin()

enable_testing()
add_subdirectory(mock_static_resources1)
add_subdirectory(test_init_resources_static_plugin)
add_subdirectory(test_static_resources_propagation)

qt_build_repo_end()
