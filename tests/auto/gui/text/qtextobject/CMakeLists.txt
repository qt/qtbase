# Generated from qtextobject.pro.

#####################################################################
## tst_qtextobject Test:
#####################################################################

qt_add_test(tst_qtextobject
    SOURCES
        tst_qtextobject.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_extend_target(tst_qtextobject CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
