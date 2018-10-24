# Generated from qdbusinterface.pro.

#####################################################################
## ../tst_qdbusinterface Test:
#####################################################################

add_qt_test(../tst_qdbusinterface
    SOURCES
        ../myobject.h
        ../tst_qdbusinterface.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::DBus
        Qt::DBusPrivate
)

## Scopes:
#####################################################################

extend_target(../tst_qdbusinterface CONDITION QT_FEATURE_dbus_linked
    DEFINES
        QT_LINKED_LIBDBUS
)

extend_target(../tst_qdbusinterface CONDITION NOT QT_FEATURE_dbus_linked
    SOURCES
        ../../../../../src/dbus/qdbus_symbols.cpp
)
