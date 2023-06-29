# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qsslsocket_onDemandCertificates_member LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qsslsocket_onDemandCertificates_member Test:
#####################################################################

qt_internal_add_test(tst_qsslsocket_onDemandCertificates_member
    SOURCES
        tst_qsslsocket_onDemandCertificates_member.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    QT_TEST_SERVER_LIST "squid" "danted"
    BUNDLE_ANDROID_OPENSSL_LIBS
)
