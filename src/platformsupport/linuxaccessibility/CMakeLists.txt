# Generated from linuxaccessibility.pro.

qt_find_package(ATSPI2) # special case

#####################################################################
## LinuxAccessibilitySupport Module:
#####################################################################

qt_add_module(LinuxAccessibilitySupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        application.cpp application_p.h
        atspiadaptor.cpp atspiadaptor_p.h
        bridge.cpp bridge_p.h
        cache.cpp cache_p.h
        constant_mappings.cpp constant_mappings_p.h
        dbusconnection.cpp dbusconnection_p.h
        struct_marshallers.cpp struct_marshallers_p.h
    DBUS_ADAPTOR_SOURCES
        dbusxml/Cache.xml
        dbusxml/DeviceEventController.xml
    DBUS_ADAPTOR_FLAGS
        "-i" "struct_marshallers_p.h"
    DBUS_INTERFACE_SOURCES
        dbusxml/Bus.xml
        dbusxml/Socket.xml
    DBUS_INTERFACE_FLAGS
        "-i" "struct_marshallers_p.h"
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        PkgConfig::ATSPI2_nolink
        Qt::CorePrivate
        Qt::DBus
        Qt::GuiPrivate
    PRECOMPILED_HEADER
        "../../corelib/global/qt_pch.h"
)

#### Keys ignored in scope 1:.:.:linuxaccessibility.pro:<TRUE>:
# MODULE = "linuxaccessibility_support"
