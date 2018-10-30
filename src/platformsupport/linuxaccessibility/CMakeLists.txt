find_package(ATSPI2 REQUIRED)

add_qt_module(LinuxAccessibilitySupport
    STATIC
    SOURCES
        application.cpp application_p.h
        atspiadaptor.cpp atspiadaptor_p.h
        bridge.cpp bridge_p.h
        cache.cpp cache_p.h
        constant_mappings.cpp constant_mappings_p.h
        dbusconnection.cpp dbusconnection_p.h
        struct_marshallers.cpp struct_marshallers_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::AccessibilitySupportPrivate
        Qt::CorePrivate
        Qt::DBus
        Qt::GuiPrivate
        PkgConfig::ATSPI2
    DBUS_ADAPTOR_SOURCES dbusxml/Cache.xml dbusxml/DeviceEventController.xml
    DBUS_ADAPTOR_FLAGS -i struct_marshallers_p.h
    DBUS_INTERFACE_SOURCES dbusxml/Socket.xml dbusxml/Bus.xml
    DBUS_INTERFACE_FLAGS -i struct_marshallers_p.h
)

