# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qabstractproxymodel Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qabstractproxymodel LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qabstractproxymodel
    SOURCES
        tst_qabstractproxymodel.cpp
    NO_BATCH # QTBUG-121815
    DEFINES
        QTEST_THROW_ON_FAIL
        QTEST_THROW_ON_SKIP
    LIBRARIES
        Qt::Gui
        Qt::TestPrivate
)
