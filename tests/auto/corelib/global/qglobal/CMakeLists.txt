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
        expansion_to_defined_check.h
        qglobal.c
        tst_qglobal.cpp
        warnings.cpp
    NO_PCH_SOURCES
        tst_qglobal.cpp # undef QT_NO_FOREACH
)

# check stuff (esp. qtypes.h) works in -ansi mode
qt_internal_add_test(tst_qglobal_strict
    SOURCES
        expansion_to_defined_check.h
        qglobal.c
        tst_qglobal.cpp
        warnings.cpp
    DEFINES
        tst_QGlobal=tst_QGlobal_strict
    NO_PCH_SOURCES
        tst_qglobal.cpp # undef QT_NO_FOREACH
)
_qt_internal_apply_strict_cpp(tst_qglobal_strict)

# enable -Werror for just the warnings.cpp TU (applies to both targets):
if (MSVC)
    set_source_files_properties(warnings.cpp PROPERTIES COMPILE_FLAGS "/WX")
else()
    set_source_files_properties(warnings.cpp PROPERTIES COMPILE_FLAGS "-Werror")
endif()

## Scopes:
#####################################################################
