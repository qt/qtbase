# Generated from windowsvista.pro.

#####################################################################
## qwindowsvistastyle Plugin:
#####################################################################

qt_add_plugin(qwindowsvistastyle
    TYPE styles
    CLASS_NAME QWindowsVistaStylePlugin
    SOURCES
        main.cpp
        qwindowsvistastyle.cpp qwindowsvistastyle_p.h
        qwindowsvistastyle_p_p.h
        qwindowsxpstyle.cpp qwindowsxpstyle_p.h
        qwindowsxpstyle_p_p.h
    LIBRARIES
        Qt::WidgetsPrivate
        gdi32
        user32
        uxtheme
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:windowsvista.pro:<TRUE>:
# DISTFILES = "windowsvistastyle.json"
