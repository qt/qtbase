# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qdbustype.pro.

#####################################################################
## tst_qdbustype Test:
#####################################################################

qt_internal_add_test(tst_qdbustype
    SOURCES
        tst_qdbustype.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qdbustype CONDITION QT_FEATURE_dbus_linked
    DEFINES
        QT_LINKED_LIBDBUS
    LIBRARIES
        dbus-1
)

qt_internal_extend_target(tst_qdbustype CONDITION NOT QT_FEATURE_dbus_linked
    SOURCES
        ../../../../src/dbus/qdbus_symbols.cpp
)
