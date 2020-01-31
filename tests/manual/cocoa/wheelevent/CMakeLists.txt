# Generated from wheelevent.pro.

#####################################################################
## wheelevent Binary:
#####################################################################

add_qt_manual_test(wheelevent
    GUI
    SOURCES
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:wheelevent.pro:<TRUE>:
# TEMPLATE = "app"
