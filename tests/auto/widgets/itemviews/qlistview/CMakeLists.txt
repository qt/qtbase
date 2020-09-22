# Generated from qlistview.pro.

#####################################################################
## tst_qlistview Test:
#####################################################################

qt_internal_add_test(tst_qlistview
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

qt_internal_extend_target(tst_qlistview CONDITION WIN32
    PUBLIC_LIBRARIES
        user32
)
