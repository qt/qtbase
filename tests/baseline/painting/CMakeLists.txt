# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_baseline_painting Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    scripts/*
    fonts/* )
list(APPEND test_data ${test_data_glob})

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_baseline_painting LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_baseline_painting
    SOURCES
        ../shared/baselineprotocol.cpp ../shared/baselineprotocol.h ../shared/lookup3.cpp
        ../shared/qbaselinetest.cpp ../shared/qbaselinetest.h
        ../shared/paintcommands.cpp ../shared/paintcommands.h
        tst_baseline_painting.cpp
    NO_PCH_SOURCES
        tst_baseline_painting.cpp # undef QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
    TESTDATA ${test_data}
)

# Resources:
set(images_resource_files
    "images/alpha.png"
    "images/alpha2x2.png"
    "images/bitmap.png"
    "images/border.png"
    "images/borderimage.png"
    "images/dome_argb32.png"
    "images/dome_indexed.png"
    "images/dome_indexed_mask.png"
    "images/dome_mono.png"
    "images/dome_mono_128.png"
    "images/dome_mono_palette.png"
    "images/dome_rgb32.png"
    "images/dot.png"
    "images/face.png"
    "images/gam030.png"
    "images/gam045.png"
    "images/gam056.png"
    "images/gam100.png"
    "images/gam200.png"
    "images/image.png"
    "images/mask.png"
    "images/mask_100.png"
    "images/masked.png"
    "images/sign.png"
    "images/solid.png"
    "images/solid2x2.png"
    "images/struct-image-01.jpg"
    "images/struct-image-01.png"
    "images/zebra.png"
)

set(fonts_resource_files
    "fonts/QtUnderlineTest-Regular.ttf"
    "fonts/QtUnderlineTest2-Regular.ttf"
)

qt_internal_add_resource(tst_baseline_painting "resources"
    PREFIX
        "/"
    FILES
        ${images_resource_files}
        ${fonts_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_baseline_painting CONDITION QT_FEATURE_opengl
    LIBRARIES
        Qt::OpenGL
)
