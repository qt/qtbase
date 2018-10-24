# Generated from services.pro.

#####################################################################
## QtServiceSupport Module:
#####################################################################

add_qt_module(ServiceSupport
    STATIC
    SOURCES
        genericunix/qgenericunixservices.cpp genericunix/qgenericunixservices_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
)

## Scopes:
#####################################################################

extend_target(ServiceSupport CONDITION QT_FEATURE_dbus
    LIBRARIES
        Qt::DBus
)
