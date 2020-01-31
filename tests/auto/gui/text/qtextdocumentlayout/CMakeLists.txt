# Generated from qtextdocumentlayout.pro.

#####################################################################
## tst_qtextdocumentlayout Test:
#####################################################################

add_qt_test(tst_qtextdocumentlayout
    SOURCES
        tst_qtextdocumentlayout.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

extend_target(tst_qtextdocumentlayout CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
