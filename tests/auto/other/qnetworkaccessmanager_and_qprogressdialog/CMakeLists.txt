# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qnetworkaccessmanager_and_qprogressdialog Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qnetworkaccessmanager_and_qprogressdialog LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qnetworkaccessmanager_and_qprogressdialog
    SOURCES
        tst_qnetworkaccessmanager_and_qprogressdialog.cpp
    LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Widgets
    QT_TEST_SERVER_LIST "apache2"
)
