# Generated from eventdispatchers.pro.

qt_find_package(GLIB2) # special case

#####################################################################
## EventDispatcherSupport Module:
#####################################################################

add_qt_module(EventDispatcherSupport
    STATIC
    INTERNAL_MODULE
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:eventdispatchers.pro:<TRUE>:
# CONFIG = "static" "internal_module"
# MODULE = "eventdispatcher_support"
# PRECOMPILED_HEADER = "../../corelib/global/qt_pch.h"
# _LOADED = "qt_module"

## Scopes:
#####################################################################

extend_target(EventDispatcherSupport CONDITION UNIX
    SOURCES
        qgenericunixeventdispatcher.cpp qgenericunixeventdispatcher_p.h
        qunixeventdispatcher.cpp
        qunixeventdispatcher_qpa_p.h
)

extend_target(EventDispatcherSupport CONDITION WIN32
    SOURCES
        qwindowsguieventdispatcher.cpp qwindowsguieventdispatcher_p.h
)

extend_target(EventDispatcherSupport CONDITION QT_FEATURE_glib
    SOURCES
        qeventdispatcher_glib.cpp qeventdispatcher_glib_p.h
    LIBRARIES
        GLIB2::GLIB2
)
