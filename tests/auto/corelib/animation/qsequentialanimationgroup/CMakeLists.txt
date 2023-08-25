# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsequentialanimationgroup Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qsequentialanimationgroup LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qsequentialanimationgroup
    SOURCES
        tst_qsequentialanimationgroup.cpp
    LIBRARIES
        Qt::TestPrivate
)
