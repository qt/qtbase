# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcombobox Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qcombobox LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
list(APPEND test_data "qtlogo.png")
list(APPEND test_data "qtlogoinverted.png")

qt_internal_add_test(tst_qcombobox
    SOURCES
        tst_qcombobox.cpp
    DEFINES
        QTEST_QPA_MOUSE_HANDLING
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
    TESTDATA ${test_data}
)
