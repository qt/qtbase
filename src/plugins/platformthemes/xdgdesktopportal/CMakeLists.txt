# Generated from xdgdesktopportal.pro.

#####################################################################
## QXdgDesktopPortalThemePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QXdgDesktopPortalThemePlugin
    OUTPUT_NAME qxdgdesktopportal
    PLUGIN_TYPE platformthemes
    DEFAULT_IF FALSE
    SOURCES
        main.cpp
        qxdgdesktopportalfiledialog.cpp qxdgdesktopportalfiledialog_p.h
        qxdgdesktopportaltheme.cpp qxdgdesktopportaltheme.h
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::DBus
        Qt::Gui
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:xdgdesktopportal.pro:<TRUE>:
# PLUGIN_EXTENDS = "-"
