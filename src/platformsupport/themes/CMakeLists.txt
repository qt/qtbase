# Generated from themes.pro.

#####################################################################
## QtThemeSupport Module:
#####################################################################

add_qt_module(ThemeSupport
    STATIC
    SOURCES
        qabstractfileiconengine.cpp qabstractfileiconengine_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
)

## Scopes:
#####################################################################
#
#extend_target(ThemeSupport CONDITION if(unix
#)

extend_target(ThemeSupport CONDITION (UNIX AND (NOT APPLE_UIKIT)) OR QT_FEATURE_xcb
    SOURCES
        genericunix/qgenericunixthemes.cpp genericunix/qgenericunixthemes_p.h
)

extend_target(ThemeSupport CONDITION ((UNIX AND (NOT APPLE_UIKIT)) OR QT_FEATURE_xcb) AND QT_FEATURE_dbus
    SOURCES
        genericunix/dbusmenu/qdbusmenuadaptor.cpp genericunix/dbusmenu/qdbusmenuadaptor.cpp genericunix/dbusmenu/qdbusmenuadaptor_p.h genericunix/dbusmenu/qdbusmenuadaptor_p.h
        genericunix/dbusmenu/qdbusmenubar.cpp genericunix/dbusmenu/qdbusmenubar.cpp genericunix/dbusmenu/qdbusmenubar_p.h genericunix/dbusmenu/qdbusmenubar_p.h
        genericunix/dbusmenu/qdbusmenuconnection.cpp genericunix/dbusmenu/qdbusmenuconnection.cpp genericunix/dbusmenu/qdbusmenuconnection_p.h genericunix/dbusmenu/qdbusmenuconnection_p.h
        genericunix/dbusmenu/qdbusmenuregistrarproxy.cpp genericunix/dbusmenu/qdbusmenuregistrarproxy.cpp genericunix/dbusmenu/qdbusmenuregistrarproxy_p.h genericunix/dbusmenu/qdbusmenuregistrarproxy_p.h
        genericunix/dbusmenu/qdbusmenutypes.cpp genericunix/dbusmenu/qdbusmenutypes.cpp genericunix/dbusmenu/qdbusmenutypes_p.h genericunix/dbusmenu/qdbusmenutypes_p.h
        genericunix/dbusmenu/qdbusplatformmenu.cpp genericunix/dbusmenu/qdbusplatformmenu.cpp genericunix/dbusmenu/qdbusplatformmenu_p.h genericunix/dbusmenu/qdbusplatformmenu_p.h
    INCLUDE_DIRECTORIES
        genericunix/dbusmenu/
        genericunix/dbusmenu/
    LIBRARIES
        Qt::DBus
)

extend_target(ThemeSupport CONDITION ((UNIX AND (NOT APPLE_UIKIT)) OR QT_FEATURE_xcb) AND QT_FEATURE_dbus AND QT_FEATURE_systemtrayicon
    SOURCES
        genericunix/dbustray/qdbustrayicon.cpp genericunix/dbustray/qdbustrayicon.cpp genericunix/dbustray/qdbustrayicon_p.h genericunix/dbustray/qdbustrayicon_p.h
        genericunix/dbustray/qdbustraytypes.cpp genericunix/dbustray/qdbustraytypes.cpp genericunix/dbustray/qdbustraytypes_p.h genericunix/dbustray/qdbustraytypes_p.h
        genericunix/dbustray/qstatusnotifieritemadaptor.cpp genericunix/dbustray/qstatusnotifieritemadaptor.cpp genericunix/dbustray/qstatusnotifieritemadaptor_p.h genericunix/dbustray/qstatusnotifieritemadaptor_p.h
        genericunix/dbustray/qxdgnotificationproxy.cpp genericunix/dbustray/qxdgnotificationproxy.cpp genericunix/dbustray/qxdgnotificationproxy_p.h genericunix/dbustray/qxdgnotificationproxy_p.h
    INCLUDE_DIRECTORIES
        genericunix/dbustray/
        genericunix/dbustray/
    LIBRARIES
        Qt::DBus
)
