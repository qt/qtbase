# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbusmarshall Test:
#####################################################################

qt_internal_add_test(tst_qdbusmarshall
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_qdbusmarshall.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
)

## Scopes:
#####################################################################

# This test case interacts with dbus code directly via the private
# headers. The include path is also not exposed via Qt:DBus.
qt_internal_extend_target(tst_qdbusmarshall CONDITION QT_FEATURE_dbus_linked
    DEFINES
        QT_LINKED_LIBDBUS
    LIBRARIES
        dbus-1
)

qt_internal_extend_target(tst_qdbusmarshall CONDITION NOT QT_FEATURE_dbus_linked
    SOURCES
        ../../../../../src/dbus/qdbus_symbols.cpp
)
