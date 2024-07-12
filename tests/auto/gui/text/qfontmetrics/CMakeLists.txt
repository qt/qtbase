# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfontmetrics Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qfontmetrics LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qfontmetrics
    SOURCES
        tst_qfontmetrics.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set_source_files_properties("../../../shared/resources/testfont.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "testfont.ttf"
)
set_source_files_properties("../../../shared/resources/testfont_linemetrics.otf"
    PROPERTIES QT_RESOURCE_ALIAS "testfont_linemetrics.otf"
)
set_source_files_properties("../../../shared/resources/testfont_bidimarks.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "testfont_bidimarks.ttf"
)
set(testfont_resource_files
    "../../../shared/resources/testfont.ttf"
    "../../../shared/resources/testfont_linemetrics.otf"
    "../../../shared/resources/testfont_bidimarks.ttf"
    "ucs4font.ttf"
)

qt_internal_add_resource(tst_qfontmetrics "testfont"
    PREFIX
        "/fonts"
    FILES
        ${testfont_resource_files}
)

