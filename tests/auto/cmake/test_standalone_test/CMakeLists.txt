# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(tststandalone_test LANGUAGES CXX)
find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)

qt_internal_add_test(tst_standalone_test
    GUI
    SOURCES
        tst_standalone_test.cpp
    LIBRARIES
        Qt::Gui
)
