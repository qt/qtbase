# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qstringview Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qstringview LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qstringview
    SOURCES
        tst_qstringview.cpp
        arrays_of_unknown_bounds.cpp arrays_of_unknown_bounds.h
    LIBRARIES
        Qt::CorePrivate
        Qt::TestPrivate
)

if(QT_FEATURE_sanitize_undefined)
    qt_internal_extend_target(tst_qstringview
        DEFINES
            QT_SANITIZE_UNDEFINED # GCC (in)famously doesn't provide a predefined macro for this
    )
endif()

## Scopes:
#####################################################################
