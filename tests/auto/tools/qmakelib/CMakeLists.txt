# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmakelib Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmakelib LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qmakelib
    SOURCES
        ../../../../qmake/library/ioutils.cpp
        ../../../../qmake/library/proitems.cpp
        ../../../../qmake/library/qmakebuiltins.cpp
        ../../../../qmake/library/qmakeevaluator.cpp
        ../../../../qmake/library/qmakeglobals.cpp
        ../../../../qmake/library/qmakeparser.cpp
        ../../../../qmake/library/qmakevfs.cpp
        ../../../../qmake/library/registry.cpp
        evaltest.cpp
        parsertest.cpp
        tst_qmakelib.cpp tst_qmakelib.h
    DEFINES
        PROEVALUATOR_FULL
        PROEVALUATOR_SETENV
        PROPARSER_DEBUG
        QT_USE_QSTRINGBUILDER
    INCLUDE_DIRECTORIES
        ../../../../qmake/library
    LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmakelib CONDITION WIN32
    LIBRARIES
        advapi32
)
