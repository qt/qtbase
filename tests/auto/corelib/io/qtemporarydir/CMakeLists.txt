# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtemporarydir Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qtemporarydir LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qtemporarydir
    SOURCES
        tst_qtemporarydir.cpp
    LIBRARIES
        Qt::TestPrivate
)

qt_internal_extend_target(tst_qtemporarydir CONDITION WIN32
    LIBRARIES
        shlwapi
)
