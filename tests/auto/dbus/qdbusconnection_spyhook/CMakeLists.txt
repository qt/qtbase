# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qdbusconnection_spyhook.pro.

#####################################################################
## tst_qdbusconnection_spyhook Test:
#####################################################################

qt_internal_add_test(tst_qdbusconnection_spyhook
    SOURCES
        ../qdbusconnection/tst_qdbusconnection.h
        tst_qdbusconnection_spyhook.cpp
    DEFINES
        tst_QDBusConnection=tst_QDBusConnection_SpyHook
    LIBRARIES
        Qt::DBus
)
