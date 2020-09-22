# Generated from qdbusabstractinterface.pro.

add_subdirectory(qpinger)

# special case begin
qt_internal_add_test(tst_qdbusabstractinterface
    SOURCES
        interface.cpp
        tst_qdbusabstractinterface.cpp
    LIBRARIES
        Qt::Core
        Qt::DBus
)

qt_internal_extend_target(tst_qdbusabstractinterface
    DBUS_INTERFACE_SOURCES
        org.qtproject.QtDBus.Pinger.xml
    DBUS_INTERFACE_BASENAME
        pinger_interface
    DBUS_INTERFACE_FLAGS
        -i interface.h
)

# special case end
