# Generated from qnetworkcookiejar.pro.

#####################################################################
## tst_qnetworkcookiejar Test:
#####################################################################

# Collect test data
list(APPEND test_data "parser.json")

add_qt_test(tst_qnetworkcookiejar
    SOURCES
        tst_qnetworkcookiejar.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
    TESTDATA ${test_data}
)
