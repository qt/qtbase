# Generated from qloggingregistry.pro.

#####################################################################
## tst_qloggingregistry Test:
#####################################################################

# Collect test data
list(APPEND test_data "qtlogging.ini")

qt_internal_add_test(tst_qloggingregistry
    SOURCES
        tst_qloggingregistry.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)
