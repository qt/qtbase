# Generated from qdbustype.pro.

#####################################################################
## tst_bench_qdbustype Binary:
#####################################################################

add_qt_benchmark(tst_bench_qdbustype
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
    PUBLIC_LIBRARIES
        Qt::DBus
        Qt::Test
)

## Scopes:
#####################################################################

extend_target(tst_bench_qdbustype CONDITION QT_FEATURE_dbus_linked
    DEFINES
        QT_LINKED_LIBDBUS
    PUBLIC_LIBRARIES
        dbus-1
)

extend_target(tst_bench_qdbustype CONDITION NOT QT_FEATURE_dbus_linked
    SOURCES
        ../../../../src/dbus/qdbus_symbols.cpp
)
