# Generated from themes.pro.

#####################################################################
## ThemeSupport Module:
#####################################################################

qt_add_module(ThemeSupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        qabstractfileiconengine.cpp qabstractfileiconengine_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:themes.pro:<TRUE>:
# MODULE = "theme_support"

## Scopes:
#####################################################################

qt_extend_target(ThemeSupport CONDITION QT_FEATURE_xcb OR (UNIX AND NOT APPLE_UIKIT)
    SOURCES
        genericunix/qgenericunixthemes.cpp genericunix/qgenericunixthemes_p.h
)

qt_extend_target(ThemeSupport CONDITION QT_FEATURE_dbus AND (QT_FEATURE_xcb OR UNIX) AND (QT_FEATURE_xcb OR NOT APPLE_UIKIT)
    SOURCES
        genericunix/dbusmenu/qdbusmenuadaptor.cpp genericunix/dbusmenu/qdbusmenuadaptor_p.h
        genericunix/dbusmenu/qdbusmenubar.cpp genericunix/dbusmenu/qdbusmenubar_p.h
        genericunix/dbusmenu/qdbusmenuconnection.cpp genericunix/dbusmenu/qdbusmenuconnection_p.h
        genericunix/dbusmenu/qdbusmenuregistrarproxy.cpp genericunix/dbusmenu/qdbusmenuregistrarproxy_p.h
        genericunix/dbusmenu/qdbusmenutypes.cpp genericunix/dbusmenu/qdbusmenutypes_p.h
        genericunix/dbusmenu/qdbusplatformmenu.cpp genericunix/dbusmenu/qdbusplatformmenu_p.h
    INCLUDE_DIRECTORIES
        genericunix/dbusmenu
    LIBRARIES
        Qt::DBus
)

qt_extend_target(ThemeSupport CONDITION QT_FEATURE_dbus AND QT_FEATURE_systemtrayicon AND (QT_FEATURE_xcb OR UNIX) AND (QT_FEATURE_xcb OR NOT APPLE_UIKIT)
    SOURCES
        genericunix/dbustray/qdbustrayicon.cpp genericunix/dbustray/qdbustrayicon_p.h
        genericunix/dbustray/qdbustraytypes.cpp genericunix/dbustray/qdbustraytypes_p.h
        genericunix/dbustray/qstatusnotifieritemadaptor.cpp genericunix/dbustray/qstatusnotifieritemadaptor_p.h
        genericunix/dbustray/qxdgnotificationproxy.cpp genericunix/dbustray/qxdgnotificationproxy_p.h
    INCLUDE_DIRECTORIES
        genericunix/dbustray
    LIBRARIES
        Qt::DBus
)
