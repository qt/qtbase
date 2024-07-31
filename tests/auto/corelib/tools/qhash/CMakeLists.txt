# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qhash Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qhash LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qhash
    SOURCES
        tst_qhash.cpp
    LIBRARIES
        Qt::TestPrivate
)

qt_internal_undefine_global_definition(tst_qhash QT_NO_JAVA_STYLE_ITERATORS)
