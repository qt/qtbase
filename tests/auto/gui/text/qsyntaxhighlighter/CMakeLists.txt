# Generated from qsyntaxhighlighter.pro.

#####################################################################
## tst_qsyntaxhighlighter Test:
#####################################################################

add_qt_test(tst_qsyntaxhighlighter
    SOURCES
        tst_qsyntaxhighlighter.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

extend_target(tst_qsyntaxhighlighter CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
