# Generated from qtextobject.pro.

#####################################################################
## tst_qtextobject Test:
#####################################################################

qt_internal_add_test(tst_qtextobject
    SOURCES
        tst_qtextobject.cpp
    LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qtextobject CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
)
