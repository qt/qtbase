# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfontengine Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qfontengine LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Resources:
set_source_files_properties("../../../shared/resources/test.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "test.ttf"
)
set_source_files_properties("../../../shared/resources/testfont.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "testfont.ttf"
)
set_source_files_properties("../../../shared/resources/testfont_variable.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "testfont_variable.ttf"
)
set(testfonts_resource_files
    "../../../shared/resources/test.ttf"
    "../../../shared/resources/testfont.ttf"
    "../../../shared/resources/testfont_variable.ttf"
)

qt_internal_add_test(tst_qfontengine
    SOURCES
        tst_qfontengine.cpp
    TESTDATA
        ${testfonts_resource_files}
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    BUILTIN_TESTDATA
)
