# Generated from qtextmarkdownimporter.pro.

#####################################################################
## tst_qtextmarkdownimporter Test:
#####################################################################

# Collect test data
list(APPEND test_data "data/thematicBreaks.md")
list(APPEND test_data "data/headingBulletsContinuations.md")
list(APPEND test_data "data/fuzz20450.md")
list(APPEND test_data "data/fuzz20580.md")

qt_internal_add_test(tst_qtextmarkdownimporter
    SOURCES
        tst_qtextmarkdownimporter.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${test_data}
)
