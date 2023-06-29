# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfontdialog Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qfontdialog LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Resources:
set_source_files_properties("../../../shared/resources/test.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "test.ttf"
)
set_source_files_properties("../../../shared/resources/testfont.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "testfont.ttf"
)
set(testfonts_resource_files
    "../../../shared/resources/test.ttf"
    "../../../shared/resources/testfont.ttf"
)

qt_internal_add_test(tst_qfontdialog
    SOURCES
        tst_qfontdialog.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
    TESTDATA ${testfonts_resource_files}
    BUILTIN_TESTDATA
)

## Scopes:
#####################################################################
