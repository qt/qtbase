# Generated from qtextobject.pro.

#####################################################################
## tst_qtextobject Test:
#####################################################################

add_qt_test(tst_qtextobject
    SOURCES
        tst_qtextobject.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

extend_target(tst_qtextobject CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
