# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qdbustype Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qdbustype
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
        Qt::Test
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_bench_qdbustype CONDITION QT_FEATURE_dbus_linked
    DEFINES
        QT_LINKED_LIBDBUS
    LIBRARIES
        dbus-1
)

qt_internal_extend_target(tst_bench_qdbustype CONDITION NOT QT_FEATURE_dbus_linked
    SOURCES
        ../../../../src/dbus/qdbus_symbols.cpp
)
