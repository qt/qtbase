# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(qmyserver)

#####################################################################
## ../tst_qdbusinterface Test:
#####################################################################

qt_internal_add_test(tst_qdbusinterface
    SOURCES
        myobject.h
        tst_qdbusinterface.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
    LIBRARIES
        Qt::Core
        Qt::DBus
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qdbusinterface CONDITION QT_FEATURE_dbus_linked
    DEFINES
        QT_LINKED_LIBDBUS
    LIBRARIES
        dbus-1
)

qt_internal_extend_target(tst_qdbusinterface CONDITION NOT QT_FEATURE_dbus_linked
    SOURCES
        ../../../../src/dbus/qdbus_symbols.cpp
)

add_dependencies(tst_qdbusinterface qmyserver_qdbusinterface)

