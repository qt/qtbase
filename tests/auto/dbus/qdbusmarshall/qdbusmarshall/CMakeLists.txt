# Generated from qdbusmarshall.pro.

#####################################################################
## ../tst_qdbusmarshall Test:
#####################################################################

add_qt_test(tst_qdbusmarshall
    SOURCES
        ../tst_qdbusmarshall.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
)

## Scopes:
#####################################################################

extend_target(tst_qdbusmarshall CONDITION QT_FEATURE_dbus_linked
    DEFINES
        QT_LINKED_LIBDBUS
)

# special case begin
# This test case interacts with dbus code directly via the private
# headers. The include path is also not exposed via Qt:DBus.
extend_target(tst_qdbusmarshall CONDITION QT_FEATURE_dbus_linked
    LIBRARIES
        dbus-1
)
# special case end

extend_target(tst_qdbusmarshall CONDITION NOT QT_FEATURE_dbus_linked
    SOURCES
        ../../../../../src/dbus/qdbus_symbols.cpp
)
