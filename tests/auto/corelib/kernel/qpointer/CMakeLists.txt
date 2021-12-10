# Generated from qpointer.pro.

#####################################################################
## tst_qpointer Test:
#####################################################################

qt_internal_add_test(tst_qpointer
    SOURCES
        tst_qpointer.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qpointer CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
