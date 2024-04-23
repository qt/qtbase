# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfuture Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qfuture LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qfuture
    SOURCES
        tst_qfuture.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::TestPrivate
)

qt_internal_extend_target(tst_qfuture CONDITION MSVC
    COMPILE_OPTIONS
        /bigobj
)

qt_internal_undefine_global_definition(tst_qfuture QT_NO_JAVA_STYLE_ITERATORS)
