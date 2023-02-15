# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
