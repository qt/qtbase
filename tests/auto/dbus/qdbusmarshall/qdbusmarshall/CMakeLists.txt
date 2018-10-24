# Generated from qdbusmarshall.pro.

#####################################################################
## ../tst_qdbusmarshall Test:
#####################################################################

add_qt_test(../tst_qdbusmarshall
    SOURCES
        ../tst_qdbusmarshall.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
)

## Scopes:
#####################################################################

extend_target(../tst_qdbusmarshall CONDITION QT_FEATURE_dbus_linked
    DEFINES
        QT_LINKED_LIBDBUS
)

extend_target(../tst_qdbusmarshall CONDITION NOT QT_FEATURE_dbus_linked
    SOURCES
        ../../../../../src/dbus/qdbus_symbols.cpp
)
