# Generated from qdbusmarshall.pro.

#####################################################################
## tst_qdbusmarshall Test:
#####################################################################

qt_add_test(tst_qdbusmarshall
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_qdbusmarshall.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
)

## Scopes:
#####################################################################

qt_extend_target(tst_qdbusmarshall CONDITION QT_FEATURE_dbus_linked
    DEFINES
        QT_LINKED_LIBDBUS
    PUBLIC_LIBRARIES
        dbus-1
)

qt_extend_target(tst_qdbusmarshall CONDITION NOT QT_FEATURE_dbus_linked
    SOURCES
        ../../../../../src/dbus/qdbus_symbols.cpp
)
