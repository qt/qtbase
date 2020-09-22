# Generated from qdbusconnection_no_libdbus.pro.

#####################################################################
## tst_qdbusconnection_no_libdbus Test:
#####################################################################

qt_internal_add_test(tst_qdbusconnection_no_libdbus
    SOURCES
        ../qdbusconnection_no_bus/tst_qdbusconnection_no_bus.cpp
    DEFINES
        SIMULATE_LOAD_FAIL
        tst_QDBusConnectionNoBus=tst_QDBusConnectionNoLibDBus1
    PUBLIC_LIBRARIES
        Qt::DBus
)
