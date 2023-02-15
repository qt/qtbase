# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(qnetworkinformation
    SOURCES
        tst_qnetworkinformation.cpp
    LIBRARIES
        Qt::Network
        Qt::Test
)

qt_internal_extend_target(qnetworkinformation CONDITION ANDROID OR UIKIT
    SOURCES
        mainwindow.h
    LIBRARIES
        Qt::Widgets
    DEFINES
        MOBILE)
