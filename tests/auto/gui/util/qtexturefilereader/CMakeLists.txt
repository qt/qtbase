# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtexturefilereader Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qtexturefilereader LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Resources:
set(qtexturefilereader_resource_files
    "texturefiles/car.ktx"
    "texturefiles/car_mips.ktx"
    "texturefiles/cubemap_float32_rgba.ktx"
    "texturefiles/cubemap_metadata.ktx"
    "texturefiles/invalid.ktx"
    "texturefiles/newlogo.astc"
    "texturefiles/newlogo_srgb.astc"
    "texturefiles/pattern.pkm"
)

qt_internal_add_test(tst_qtexturefilereader
    SOURCES
        tst_qtexturefilereader.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${qtexturefilereader_resource_files}
    BUILTIN_TESTDATA
)

