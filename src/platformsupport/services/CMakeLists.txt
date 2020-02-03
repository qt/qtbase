# Generated from services.pro.

#####################################################################
## ServiceSupport Module:
#####################################################################

qt_add_module(ServiceSupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        genericunix/qgenericunixservices.cpp genericunix/qgenericunixservices_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PRECOMPILED_HEADER
        "../../corelib/global/qt_pch.h"
)

#### Keys ignored in scope 1:.:.:services.pro:<TRUE>:
# MODULE = "service_support"

## Scopes:
#####################################################################

qt_extend_target(ServiceSupport CONDITION QT_FEATURE_dbus
    PUBLIC_LIBRARIES
        Qt::DBus
)
