# Generated from qhttpnetworkconnection.pro.

#####################################################################
## tst_qhttpnetworkconnection Test:
#####################################################################

add_qt_test(tst_qhttpnetworkconnection
    GUI
    SOURCES
        tst_qhttpnetworkconnection.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:qhttpnetworkconnection.pro:<TRUE>:
# CONFIG = "testcase" "unsupported/testserver"
# QT_TEST_SERVER_LIST = "apache2"
