# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qrawfont Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qrawfont LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Resources:
set_source_files_properties("../../../shared/resources/testfont.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "testfont.ttf"
)
set(testdata_resource_files
    "../../../shared/resources/testfont.ttf"
    "testfont_zeroem.ttf"
    "testfont_bold_italic.ttf"
    "testfont_os2_v1.ttf"
)

qt_internal_add_test(tst_qrawfont
    SOURCES
        tst_qrawfont.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${testdata_resource_files}
    BUILTIN_TESTDATA
)

