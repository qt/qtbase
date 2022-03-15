# Generated from qnetworkinterface.pro.

#####################################################################
## tst_qnetworkinterface Test:
#####################################################################

qt_internal_add_test(tst_qnetworkinterface
    SOURCES
        tst_qnetworkinterface.cpp
    PUBLIC_LIBRARIES
        Qt::Network
    QT_TEST_SERVER_LIST "apache2"
)
