# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(asyncify_exec LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(asyncify_exec
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
)

# Enable asyncify for this test. Also enable optimizations in order to reduce the binary size.
target_link_options(asyncify_exec PUBLIC -sASYNCIFY -Os)
