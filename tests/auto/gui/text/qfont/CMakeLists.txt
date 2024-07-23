# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfont Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qfont LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Resources:
set_source_files_properties("../../../shared/resources/testfont_variable.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "testfont_variable.ttf"
)

set(testfont_resource_files
    "datastream.515"
    "weirdfont.otf"
    "../../../shared/resources/testfont_variable.ttf"
)

qt_internal_add_test(tst_qfont
    SOURCES
        tst_qfont.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
    TESTDATA ${testfont_resource_files}
    BUILTIN_TESTDATA
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qfont CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
)
