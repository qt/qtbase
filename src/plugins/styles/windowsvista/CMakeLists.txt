# Generated from windowsvista.pro.

#####################################################################
## QWindowsVistaStylePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QWindowsVistaStylePlugin
    OUTPUT_NAME qwindowsvistastyle
    PLUGIN_TYPE styles
    SOURCES
        main.cpp
        qwindowsvistastyle.cpp qwindowsvistastyle_p.h
        qwindowsvistastyle_p_p.h
        qwindowsxpstyle.cpp qwindowsxpstyle_p.h
        qwindowsxpstyle_p_p.h
    LIBRARIES
        gdi32
        user32
        uxtheme
        Qt::Core
        Qt::Gui
        Qt::WidgetsPrivate
)

#### Keys ignored in scope 1:.:.:windowsvista.pro:<TRUE>:
# DISTFILES = "windowsvistastyle.json"
