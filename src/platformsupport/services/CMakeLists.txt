# Generated from services.pro.

#####################################################################
## ServiceSupport Module:
#####################################################################

add_qt_module(ServiceSupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        genericunix/qgenericunixservices.cpp genericunix/qgenericunixservices_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:services.pro:<TRUE>:
# CONFIG = "static" "internal_module"
# MODULE = "service_support"
# PRECOMPILED_HEADER = "../../corelib/global/qt_pch.h"
# _LOADED = "qt_module"

## Scopes:
#####################################################################

extend_target(ServiceSupport CONDITION QT_FEATURE_dbus
    PUBLIC_LIBRARIES
        Qt::DBus
)
