# Generated from xdgdesktopportal.pro.

#####################################################################
## qxdgdesktopportal Plugin:
#####################################################################

qt_add_plugin(qxdgdesktopportal
    TYPE platformthemes
    CLASS_NAME QXdgDesktopPortalThemePlugin
    SOURCES
        main.cpp
        qxdgdesktopportalfiledialog.cpp qxdgdesktopportalfiledialog_p.h
        qxdgdesktopportaltheme.cpp qxdgdesktopportaltheme.h
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::ThemeSupportPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::DBus
        Qt::Gui
        Qt::ThemeSupport
)

#### Keys ignored in scope 1:.:.:xdgdesktopportal.pro:<TRUE>:
# PLUGIN_EXTENDS = "-"
