# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qopengl Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qopengl LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qopengl
    SOURCES
        tst_qopengl.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGL
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qopengl CONDITION LINUX AND QT_FEATURE_xcb AND QT_FEATURE_xcb_glx_plugin
    DEFINES
        USE_GLX
)
