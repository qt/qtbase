# Generated from qsavefile.pro.

#####################################################################
## tst_qsavefile Test:
#####################################################################

# Collect test data
list(APPEND test_data "tst_qsavefile.cpp")

qt_internal_add_test(tst_qsavefile
    SOURCES
        tst_qsavefile.cpp
    TESTDATA ${test_data}
)
