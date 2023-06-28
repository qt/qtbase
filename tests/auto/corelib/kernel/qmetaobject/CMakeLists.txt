# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmetaobject LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

set(tst_qmetaobject_SOURCES
    tst_qmetaobject.cpp
    forwarddeclared.h
    forwarddeclared.cpp
)


#####################################################################
## tst_qmetaobject Test:
#####################################################################

qt_internal_add_test(tst_qmetaobject
    SOURCES
        ${tst_qmetaobject_SOURCES}
    LIBRARIES
        Qt::CorePrivate
)

qt_internal_add_test(tst_qmetaobject_compat
    SOURCES
        ${tst_qmetaobject_SOURCES}
    DEFINES
        USE_COMPAT_Q_ARG=1
    LIBRARIES
        Qt::CorePrivate
    NO_BATCH
)
