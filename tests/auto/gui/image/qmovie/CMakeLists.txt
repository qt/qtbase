# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmovie Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmovie LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    animations/* multiframe/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmovie
    SOURCES
        tst_qmovie.cpp
    LIBRARIES
        Qt::Gui
        Qt::TestPrivate
    TESTDATA ${test_data}
)

# Resources:
set(resources_resource_files
    "animations/comicsecard.gif"
    "animations/corrupt.gif"
    "animations/trolltech.gif"
    "multiframe/Obj_N2_Internal_Mem.ico"
)

qt_internal_add_resource(tst_qmovie "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmovie CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
)

qt_internal_extend_target(tst_qmovie CONDITION QT_FEATURE_gif
    DEFINES
        QTEST_HAVE_GIF
)

qt_internal_extend_target(tst_qmovie CONDITION QT_FEATURE_jpeg
    DEFINES
        QTEST_HAVE_JPEG
)
