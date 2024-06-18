# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qformdatabuilder LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qformdatabuilder
    NO_BATCH # QTBUG-121815
    DEFINES
        QTEST_THROW_ON_FAIL
        QTEST_THROW_ON_SKIP
    SOURCES
        tst_qformdatabuilder.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Network
        Qt::NetworkPrivate
    TESTDATA
        rfc3252.txt
        image1.jpg
        document.docx
        sheet.xlsx
)

if(QT_FEATURE_sanitize_undefined)
    qt_internal_extend_target(tst_qformdatabuilder
        DEFINES
            QT_SANITIZE_UNDEFINED # GCC (in)famously doesn't provide a predefined macro for this
    )
endif()
