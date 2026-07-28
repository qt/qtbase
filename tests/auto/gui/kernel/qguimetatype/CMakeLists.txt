# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qguimetatype Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qguimetatype LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qguimetatype
    SOURCES
        tst_qguimetatype.cpp
    LIBRARIES
        Qt::Gui
)

if(QT_FEATURE_sanitize_undefined)
    qt_internal_extend_target(tst_qguimetatype
        DEFINES
            QT_SANITIZE_UNDEFINED # GCC (in)famously doesn't provide a predefined macro for this
    )
endif()
