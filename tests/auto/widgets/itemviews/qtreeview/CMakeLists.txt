# Generated from qtreeview.pro.

#####################################################################
## tst_qtreeview Test:
#####################################################################

add_qt_test(tst_qtreeview
    SOURCES
        ../../../../shared/fakedirmodel.h
        tst_qtreeview.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)
