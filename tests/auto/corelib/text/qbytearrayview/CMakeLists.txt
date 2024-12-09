# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qbytearrayview Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qbytearrayview LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qbytearrayview
    SOURCES
        tst_qbytearrayview.cpp
        ../qstringview/arrays_of_unknown_bounds.cpp
        ../qstringview/arrays_of_unknown_bounds.h
)

if(QT_FEATURE_sanitize_undefined)
    qt_internal_extend_target(tst_qbytearrayview
        DEFINES
            QT_SANITIZE_UNDEFINED # GCC (in)famously doesn't provide a predefined macro for this
    )
endif()
