# Generated from qdbustype.pro.

#####################################################################
## tst_qdbustype Test:
#####################################################################

add_qt_test(tst_qdbustype
    SOURCES
        tst_qdbustype.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::DBus
)

#### Keys ignored in scope 1:.:.:qdbustype.pro:<TRUE>:
# CONFIG = "testcase" "parallel_test"

## Scopes:
#####################################################################

 extend_target(tst_qdbustype CONDITION QT_FEATURE_dbus_linked
     DEFINES
         QT_LINKED_LIBDBUS
     LIBRARIES
         dbus-1
 )

 extend_target(tst_qdbustype CONDITION NOT QT_FEATURE_dbus_linked
     SOURCES
         ../../../../src/dbus/qdbus_symbols.cpp
 )
