# Generated from gtk3.pro.

qt_find_package(GTK3)
qt_find_package(X11)

#####################################################################
## qgtk3 Plugin:
#####################################################################

add_qt_plugin(qgtk3
    TYPE platformthemes
    CLASS_NAME QGtk3ThemePlugin
    SOURCES
        main.cpp
        qgtk3dialoghelpers.cpp qgtk3dialoghelpers.h
        qgtk3menu.cpp qgtk3menu.h
        qgtk3theme.cpp qgtk3theme.h
    DEFINES
        GDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_6
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::ThemeSupportPrivate
        PkgConfig::GTK3
        X11::X11
    # CONFIG = "X11"
    # PLUGIN_CLASS_NAME = "QGtk3ThemePlugin"
    # PLUGIN_EXTENDS = "-"
    # _LOADED = "qt_plugin"
)
