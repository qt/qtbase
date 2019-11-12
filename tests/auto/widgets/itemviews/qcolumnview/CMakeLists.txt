# Generated from qcolumnview.pro.

#####################################################################
## tst_qcolumnview Test:
#####################################################################

add_qt_test(tst_qcolumnview
    SOURCES
        ../../../../shared/fakedirmodel.h
        tst_qcolumnview.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)
