# Generated from qlistview.pro.

#####################################################################
## tst_qlistview Test:
#####################################################################

add_qt_test(tst_qlistview
    SOURCES
        tst_qlistview.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

## Scopes:
#####################################################################

extend_target(tst_qlistview CONDITION WIN32 AND NOT WINRT
    PUBLIC_LIBRARIES
        user32
)
