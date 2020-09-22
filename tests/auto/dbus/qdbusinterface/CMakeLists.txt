# Generated from qdbusinterface.pro.

add_subdirectory(qmyserver)
# special case begin

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
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::DBus
)

#### Keys ignored in scope 1:.:.:qdbusinterface.pro:<TRUE>:
# CONFIG = "testcase"
# DESTDIR = "./"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qdbusinterface CONDITION QT_FEATURE_dbus_linked
    DEFINES
        QT_LINKED_LIBDBUS
    PUBLIC_LIBRARIES
        dbus-1
)

qt_internal_extend_target(tst_qdbusinterface CONDITION NOT QT_FEATURE_dbus_linked
    SOURCES
        ../../../../src/dbus/qdbus_symbols.cpp
)

# special case end
