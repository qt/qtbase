# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qglobal Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qglobal LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qglobal
    SOURCES
        qglobal.c
        tst_qglobal.cpp
    NO_PCH_SOURCES
        tst_qglobal.cpp # undef QT_NO_FOREACH
)

# check stuff (esp. qtypes.h) works in -ansi mode
qt_internal_add_test(tst_qglobal_strict
    SOURCES
        qglobal.c
        tst_qglobal.cpp
    DEFINES
        tst_QGlobal=tst_QGlobal_strict
    NO_PCH_SOURCES
        tst_qglobal.cpp # undef QT_NO_FOREACH
)
_qt_internal_apply_strict_cpp(tst_qglobal_strict)

## Scopes:
#####################################################################
