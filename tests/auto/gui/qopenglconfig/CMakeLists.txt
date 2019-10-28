# Generated from qopenglconfig.pro.

#####################################################################
## tst_qopenglconfig Test:
#####################################################################

# Collect test data
list(APPEND test_data "buglist.json")

add_qt_test(tst_qopenglconfig
    SOURCES
        tst_qopenglconfig.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)
