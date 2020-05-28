# Generated from eventdispatchers.pro.

qt_find_package(GLIB2) # special case

#####################################################################
## EventDispatcherSupport Module:
#####################################################################

qt_add_module(EventDispatcherSupport
    STATIC
    INTERNAL_MODULE
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PRECOMPILED_HEADER
        "../../corelib/global/qt_pch.h"
)

#### Keys ignored in scope 1:.:.:eventdispatchers.pro:<TRUE>:
# MODULE = "eventdispatcher_support"

## Scopes:
#####################################################################

qt_extend_target(EventDispatcherSupport CONDITION UNIX
    SOURCES
        qgenericunixeventdispatcher.cpp qgenericunixeventdispatcher_p.h
        qunixeventdispatcher.cpp
        qunixeventdispatcher_qpa_p.h
)

qt_extend_target(EventDispatcherSupport CONDITION QT_FEATURE_glib
    SOURCES
        qeventdispatcher_glib.cpp qeventdispatcher_glib_p.h
    LIBRARIES
        GLIB2::GLIB2
)
