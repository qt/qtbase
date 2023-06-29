# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qsslsocket LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qsslsocket Test:
#####################################################################

# Collect test data
list(APPEND test_data "certs")

qt_internal_add_test(tst_qsslsocket
    SOURCES
        tst_qsslsocket.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
        Qt::TestPrivate
    TESTDATA ${test_data}
    QT_TEST_SERVER_LIST "squid" "danted" "cyrus" "apache2" "echo"
    BUNDLE_ANDROID_OPENSSL_LIBS
)

qt_internal_extend_target(tst_qsslsocket CONDITION QT_FEATURE_private_tests AND QT_FEATURE_openssl_linked
    LIBRARIES
        WrapOpenSSL::WrapOpenSSL
)
