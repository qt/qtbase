# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwidget Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qwidget LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Resources:
set(qwidget_resource_files
    "geometry-fullscreen.dat"
    "geometry-maximized.dat"
    "geometry.dat"
    "hellotr_la.qm"
)

qt_internal_add_test(tst_qwidget
    SOURCES
        tst_qwidget.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
    TESTDATA ${qwidget_resource_files}
    BUILTIN_TESTDATA
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qwidget CONDITION AIX
    COMPILE_OPTIONS
        -fpermissive
)

qt_internal_extend_target(tst_qwidget CONDITION WIN32
    LIBRARIES
        gdi32
        user32
)
