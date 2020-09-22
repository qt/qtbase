# Generated from qsplitter.pro.

#####################################################################
## tst_qsplitter Test:
#####################################################################

# Collect test data
list(APPEND test_data "extradata.txt")
list(APPEND test_data "setSizes3.dat")

qt_internal_add_test(tst_qsplitter
    SOURCES
        tst_qsplitter.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
    TESTDATA ${test_data}
)
