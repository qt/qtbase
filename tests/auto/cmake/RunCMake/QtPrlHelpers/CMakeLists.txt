# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(${RunCMake_TEST} LANGUAGES CXX)

find_package(Qt6 COMPONENTS BuildInternals)

# QtGenerateLibHelpers and QtFinishPrlFile are not auto-loaded by
# Qt6BuildInternals; the test scripts include them explicitly.
get_filename_component(QT_PRL_HELPERS_QTBASE_CMAKE_DIR "${Qt6_DIR}" ABSOLUTE)
include(${RunCMake_TEST}.cmake)
