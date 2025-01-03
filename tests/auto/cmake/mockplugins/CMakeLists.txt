# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)
project(QtMockPlugins
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt MockPlugins Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals CorePrivate)
qt_internal_project_setup()

find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Gui Widgets Xml)

qt_build_repo_begin()

add_subdirectory(mockplugins1)
add_subdirectory(mockplugins2)
add_subdirectory(mockplugins3)

add_subdirectory(mock1plugin)
add_subdirectory(mock2plugin)
add_subdirectory(mock3plugin)
add_subdirectory(mock4plugin)
add_subdirectory(mock5plugin)
add_subdirectory(mock6plugin)

qt_build_repo_post_process()
qt_build_repo_end()
