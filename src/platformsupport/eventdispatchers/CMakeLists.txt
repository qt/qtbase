# Generated from eventdispatchers.pro.

find_package(GLIB2)
set_package_properties(GLib PROPERTIES TYPE OPTIONAL)

#####################################################################
## QtEventDispatcherSupport Module:
#####################################################################

add_qt_module(EventDispatcherSupport
    STATIC
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
)

## Scopes:
#####################################################################

extend_target(EventDispatcherSupport CONDITION UNIX
    SOURCES
        qgenericunixeventdispatcher.cpp qgenericunixeventdispatcher_p.h
        qunixeventdispatcher.cpp
        qunixeventdispatcher_qpa_p.h
)

extend_target(EventDispatcherSupport CONDITION NOT UNIX
    SOURCES
        qwindowsguieventdispatcher.cpp qwindowsguieventdispatcher_p.h
)

extend_target(EventDispatcherSupport CONDITION QT_FEATURE_glib
    SOURCES
        qeventdispatcher_glib.cpp qeventdispatcher_glib_p.h
    LIBRARIES
        GLIB2::GLIB2
)
