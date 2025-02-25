# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qurl Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qurl LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qurl
    SOURCES
        tst_qurl.cpp
    LIBRARIES
        Qt::Concurrent
        Qt::TestPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qurl CONDITION APPLE
    SOURCES
        tst_qurl_mac.mm
    LIBRARIES
        Qt::CorePrivate
)
