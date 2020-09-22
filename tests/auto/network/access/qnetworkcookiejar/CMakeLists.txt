# Generated from qnetworkcookiejar.pro.

#####################################################################
## tst_qnetworkcookiejar Test:
#####################################################################

# Collect test data
list(APPEND test_data "parser.json")

qt_internal_add_test(tst_qnetworkcookiejar
    SOURCES
        tst_qnetworkcookiejar.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::NetworkPrivate
    TESTDATA ${test_data}
)
