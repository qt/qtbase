# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qgraphicsscene Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qgraphicsscene LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Resources:

set(testdata_resource_files
    "Ash_European.jpg"
    "testData/render/all-all-45-deg-left.png"
    "testData/render/all-all-45-deg-right.png"
    "testData/render/all-all-scale-2x.png"
    "testData/render/all-all-translate-0-50.png"
    "testData/render/all-all-translate-50-0.png"
    "testData/render/all-all-untransformed-clip-ellipse.png"
    "testData/render/all-all-untransformed-clip-rect.png"
    "testData/render/all-all-untransformed.png"
    "testData/render/all-bottomleft-untransformed.png"
    "testData/render/all-bottomright-untransformed.png"
    "testData/render/all-topleft-untransformed.png"
    "testData/render/all-topright-untransformed.png"
    "testData/render/bottom-bottomright-untransformed.png"
    "testData/render/bottom-topleft-untransformed.png"
    "testData/render/bottomleft-all-untransformed.png"
    "testData/render/bottomleft-topleft-untransformed.png"
    "testData/render/bottomright-all-untransformed.png"
    "testData/render/bottomright-topleft-untransformed.png"
    "testData/render/left-bottomright-untransformed.png"
    "testData/render/left-topleft-untransformed.png"
    "testData/render/right-bottomright-untransformed.png"
    "testData/render/right-topleft-untransformed.png"
    "testData/render/top-bottomright-untransformed.png"
    "testData/render/top-topleft-untransformed.png"
    "testData/render/topleft-all-untransformed.png"
    "testData/render/topleft-topleft-untransformed.png"
    "testData/render/topright-all-untransformed.png"
    "testData/render/topright-topleft-untransformed.png"
)

qt_internal_add_test(tst_qgraphicsscene
    LOWDPI
    SOURCES
        tst_qgraphicsscene.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
    TESTDATA ${testdata_resource_files}
    BUILTIN_TESTDATA
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qgraphicsscene CONDITION WIN32
    LIBRARIES
        user32
)
