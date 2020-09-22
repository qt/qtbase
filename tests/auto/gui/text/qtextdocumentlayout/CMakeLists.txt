# Generated from qtextdocumentlayout.pro.

#####################################################################
## tst_qtextdocumentlayout Test:
#####################################################################

qt_internal_add_test(tst_qtextdocumentlayout
    SOURCES
        tst_qtextdocumentlayout.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qtextdocumentlayout CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
