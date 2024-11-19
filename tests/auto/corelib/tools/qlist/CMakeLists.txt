# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qlist Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qlist LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qlist
    SOURCES
        tst_qlist.cpp
    DEFINES
        QT_NO_STRICT_QLIST_ITERATORS # doesn't exist, except for this test
    LIBRARIES
        Qt::CorePrivate
)

qt_internal_add_test(tst_qlist_strict_iterators
    SOURCES
        tst_qlist.cpp
    DEFINES
        QT_STRICT_QLIST_ITERATORS
        tst_QList=tst_QList_strict_iterators
    LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################
