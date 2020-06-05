# Generated from qlistview.pro.

#####################################################################
## tst_qlistview Test:
#####################################################################

add_qt_test(tst_qlistview
    SOURCES
        tst_qlistview.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

extend_target(tst_qlistview CONDITION WIN32
    PUBLIC_LIBRARIES
        user32
)
