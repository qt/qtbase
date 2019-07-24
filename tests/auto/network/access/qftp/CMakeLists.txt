# Generated from qftp.pro.

#####################################################################
## tst_qftp Test:
#####################################################################

add_qt_test(tst_qftp
    GUI
    SOURCES
        tst_qftp.cpp
    LIBRARIES
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:qftp.pro:<TRUE>:
# CONFIG = "testcase" "unsupported/testserver"
# QT_TEST_SERVER_LIST = "vsftpd" "ftp-proxy" "squid" "danted"
