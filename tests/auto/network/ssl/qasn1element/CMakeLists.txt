# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qasn1element Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qasn1element LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qasn1element
    SOURCES
        tst_qasn1element.cpp
        ../../../../../src/plugins/tls/shared/qasn1element_p.h
        ../../../../../src/plugins/tls/shared/qasn1element.cpp
    INCLUDE_DIRECTORIES
        ../../../../../src/plugins/tls/shared
    LIBRARIES
        Qt::Core
        Qt::Network
        Qt::NetworkPrivate
    BUNDLE_ANDROID_OPENSSL_LIBS
)
