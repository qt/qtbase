# Generated from qdbusconnection_spyhook.pro.

#####################################################################
## tst_qdbusconnection_spyhook Test:
#####################################################################

qt_internal_add_test(tst_qdbusconnection_spyhook
    SOURCES
        ../qdbusconnection/tst_qdbusconnection.h
        tst_qdbusconnection_spyhook.cpp
    DEFINES
        tst_QDBusConnection=tst_QDBusConnection_SpyHook
    PUBLIC_LIBRARIES
        Qt::DBus
)
