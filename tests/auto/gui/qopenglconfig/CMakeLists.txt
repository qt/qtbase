# Generated from qopenglconfig.pro.

#####################################################################
## tst_qopenglconfig Test:
#####################################################################

# Collect test data
list(APPEND test_data "buglist.json")

qt_internal_add_test(tst_qopenglconfig
    SOURCES
        tst_qopenglconfig.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${test_data}
)
