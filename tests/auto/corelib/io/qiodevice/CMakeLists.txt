# Generated from qiodevice.pro.

#####################################################################
## tst_qiodevice Test:
#####################################################################

# Collect test data
list(APPEND test_data "tst_qiodevice.cpp")

qt_internal_add_test(tst_qiodevice
    SOURCES
        tst_qiodevice.cpp
    PUBLIC_LIBRARIES
        Qt::Network
    TESTDATA ${test_data}
    QT_TEST_SERVER_LIST "apache2" "cyrus"
)
