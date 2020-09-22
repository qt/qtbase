# Generated from qsyntaxhighlighter.pro.

#####################################################################
## tst_qsyntaxhighlighter Test:
#####################################################################

qt_internal_add_test(tst_qsyntaxhighlighter
    SOURCES
        tst_qsyntaxhighlighter.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qsyntaxhighlighter CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
