# Generated from xdgdesktopportal.pro.

#####################################################################
## QXdgDesktopPortalThemePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QXdgDesktopPortalThemePlugin
    OUTPUT_NAME qxdgdesktopportal
    TYPE platformthemes
    SOURCES
        main.cpp
        qxdgdesktopportalfiledialog.cpp qxdgdesktopportalfiledialog_p.h
        qxdgdesktopportaltheme.cpp qxdgdesktopportaltheme.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::DBus
        Qt::Gui
        Qt::GuiPrivate
        Qt::ThemeSupportPrivate
)

#### Keys ignored in scope 1:.:.:xdgdesktopportal.pro:<TRUE>:
# PLUGIN_EXTENDS = "-"
