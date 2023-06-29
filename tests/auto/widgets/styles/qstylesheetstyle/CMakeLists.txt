# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qstylesheetstyle Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qstylesheetstyle LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Resources:
set(resources_resource_files
    "images/testimage.png"
    "images/testimage@2x.png"
)

qt_internal_add_test(tst_qstylesheetstyle
    SOURCES
        tst_qstylesheetstyle.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
    TESTDATA ${resources_resource_files}
    BUILTIN_TESTDATA
)

# _REQUIREMENTS = "qtConfig(private_tests)"
