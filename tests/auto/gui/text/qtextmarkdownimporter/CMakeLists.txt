# Generated from qtextmarkdownimporter.pro.

#####################################################################
## tst_qtextmarkdownimporter Test:
#####################################################################

# Collect test data
list(APPEND test_data "data/thematicBreaks.md")
list(APPEND test_data "data/headingBulletsContinuations.md")

add_qt_test(tst_qtextmarkdownimporter
    SOURCES
        tst_qtextmarkdownimporter.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)
