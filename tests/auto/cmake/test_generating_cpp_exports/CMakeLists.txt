# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)

project(TestGeneratingCppExports
        DESCRIPTION "Test of the generating of cpp exports"
        HOMEPAGE_URL "https://qt.io/"
        LANGUAGES CXX C
        VERSION "${QT_REPO_MODULE_VERSION}"
)

find_package(Qt6 COMPONENTS Core BuildInternals Test CONFIG REQUIRED)
qt_internal_project_setup()

qt_build_repo_begin()

add_subdirectory(test_autogenerating_cpp_exports)
add_subdirectory(test_autogenerating_cpp_exports_custom_name)

qt_build_repo_post_process()
qt_build_repo_end()
