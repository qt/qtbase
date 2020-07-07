# Generated from qtextdocumentlayout.pro.

#####################################################################
## tst_qtextdocumentlayout Test:
#####################################################################

qt_add_test(tst_qtextdocumentlayout
    SOURCES
        tst_qtextdocumentlayout.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_extend_target(tst_qtextdocumentlayout CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
