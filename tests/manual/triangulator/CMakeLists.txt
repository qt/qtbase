# Generated from triangulator.pro.

#####################################################################
## triangulator Binary:
#####################################################################

add_qt_manual_test(triangulator
    GUI
    SOURCES
        main.cpp
        triviswidget.cpp triviswidget.h
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:triangulator.pro:<TRUE>:
# TEMPLATE = "app"
