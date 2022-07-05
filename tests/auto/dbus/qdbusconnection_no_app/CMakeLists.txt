# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qdbusconnection_no_app.pro.

#####################################################################
## tst_qdbusconnection_no_app Test:
#####################################################################

qt_internal_add_test(tst_qdbusconnection_no_app
    SOURCES
        ../qdbusconnection/tst_qdbusconnection.h
        tst_qdbusconnection_no_app.cpp
    DEFINES
        tst_QDBusConnection=tst_QDBusConnection_NoApplication
    LIBRARIES
        Qt::DBus
)
