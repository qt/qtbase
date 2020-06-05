# Generated from qitemdelegate.pro.

#####################################################################
## tst_qitemdelegate Test:
#####################################################################

add_qt_test(tst_qitemdelegate
    SOURCES
        tst_qitemdelegate.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

extend_target(tst_qitemdelegate CONDITION WIN32
    PUBLIC_LIBRARIES
        user32
)
