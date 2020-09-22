# Generated from qitemdelegate.pro.

#####################################################################
## tst_qitemdelegate Test:
#####################################################################

qt_internal_add_test(tst_qitemdelegate
    SOURCES
        tst_qitemdelegate.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qitemdelegate CONDITION WIN32
    PUBLIC_LIBRARIES
        user32
)
