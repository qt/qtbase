# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdatetime Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qdatetime LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qdatetime
    SOURCES
        tst_qdatetime.cpp
    DEFINES
        QT_NO_FOREACH
        QT_NO_KEYWORDS
    LIBRARIES
        Qt::CorePrivate
        Qt::TestPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qdatetime CONDITION APPLE
    SOURCES
        tst_qdatetime_mac.mm
    LIBRARIES
        ${FWFoundation}
)
