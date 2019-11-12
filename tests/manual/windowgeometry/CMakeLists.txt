# Generated from windowgeometry.pro.

#####################################################################
## windowgeometry Binary:
#####################################################################

add_qt_manual_test(windowgeometry
    GUI
    SOURCES
        ../windowflags/controls.cpp ../windowflags/controls.h
        controllerwidget.cpp controllerwidget.h
        main.cpp
    INCLUDE_DIRECTORIES
        ../windowflags
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:windowgeometry.pro:<TRUE>:
# TEMPLATE = "app"
