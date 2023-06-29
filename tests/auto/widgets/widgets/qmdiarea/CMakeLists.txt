# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmdiarea Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmdiarea LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qmdiarea
    SOURCES
        tst_qmdiarea.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmdiarea CONDITION TARGET Qt::OpenGL
    LIBRARIES
        Qt::OpenGL
)

qt_internal_extend_target(tst_qmdiarea CONDITION APPLE
    LIBRARIES
        ${FWSecurity}
)
