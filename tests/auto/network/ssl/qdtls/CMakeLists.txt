# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdtls Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qdtls LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
list(APPEND test_data "certs")

qt_internal_add_test(tst_qdtls
    SOURCES
        tst_qdtls.cpp
    LIBRARIES
        Qt::NetworkPrivate
    TESTDATA ${test_data}
    BUNDLE_ANDROID_OPENSSL_LIBS
)
