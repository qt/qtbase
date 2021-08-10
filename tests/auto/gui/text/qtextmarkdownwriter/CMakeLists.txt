# Generated from qtextmarkdownwriter.pro.

#####################################################################
## tst_qtextmarkdownwriter Test:
#####################################################################

# Collect test data
list(APPEND test_data "data/example.md")
list(APPEND test_data "data/blockquotes.md")

qt_internal_add_test(tst_qtextmarkdownwriter
    SOURCES
        tst_qtextmarkdownwriter.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${test_data}
)
