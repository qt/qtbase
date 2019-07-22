# Generated from xdgdesktopportal.pro.

#####################################################################
## qxdgdesktopportal Plugin:
#####################################################################

add_qt_plugin(qxdgdesktopportal
    TYPE platformthemes
    CLASS_NAME QXdgDesktopPortalThemePlugin
    SOURCES
        main.cpp
        qxdgdesktopportalfiledialog.cpp qxdgdesktopportalfiledialog_p.h
        qxdgdesktopportaltheme.cpp qxdgdesktopportaltheme.h
    LIBRARIES
        Qt::CorePrivate
        Qt::DBus
        Qt::GuiPrivate
        Qt::ThemeSupportPrivate
    # PLUGIN_CLASS_NAME = "QXdgDesktopPortalThemePlugin"
    # PLUGIN_EXTENDS = "-"
    # _LOADED = "qt_plugin"
)
