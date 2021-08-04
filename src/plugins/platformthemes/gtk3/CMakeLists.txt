# Generated from gtk3.pro.

qt_find_package(GTK3) # special case
qt_find_package(X11) # special case

#####################################################################
## QGtk3ThemePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QGtk3ThemePlugin
    OUTPUT_NAME qgtk3
    PLUGIN_TYPE platformthemes
    DEFAULT_IF FALSE
    SOURCES
        main.cpp
        qgtk3dialoghelpers.cpp qgtk3dialoghelpers.h
        qgtk3menu.cpp qgtk3menu.h
        qgtk3theme.cpp qgtk3theme.h
    DEFINES
        GDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_6
    LIBRARIES # special case
        PkgConfig::GTK3
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        X11::X11 # special case
)

#### Keys ignored in scope 1:.:.:gtk3.pro:<TRUE>:
# PLUGIN_EXTENDS = "-"
