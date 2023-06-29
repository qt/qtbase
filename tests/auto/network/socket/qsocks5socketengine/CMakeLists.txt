# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qsocks5socketengine LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qsocks5socketengine Test:
#####################################################################

qt_internal_add_test(tst_qsocks5socketengine
    SOURCES
        tst_qsocks5socketengine.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    QT_TEST_SERVER_LIST "danted" "apache2" "cyrus"
)

## Scopes:
#####################################################################

# QT_TEST_SERVER_LIST = "danted" "apache2" "cyrus"

qt_internal_extend_target(tst_qsocks5socketengine CONDITION WIN32
    LIBRARIES
        ws2_32
)
