# Generated from qtexttable.pro.

#####################################################################
## tst_qtexttable Test:
#####################################################################

add_qt_test(tst_qtexttable
    SOURCES
        tst_qtexttable.cpp
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

extend_target(tst_qtexttable CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
