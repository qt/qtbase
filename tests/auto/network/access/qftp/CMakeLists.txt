# Generated from qftp.pro.

if(NOT QT_FEATURE_ftp)
    return()
endif()
if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qftp Test:
#####################################################################

add_qt_test(tst_qftp
    SOURCES
        tst_qftp.cpp
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::NetworkPrivate
)

#### Keys ignored in scope 1:.:.:qftp.pro:<TRUE>:
# QT_FOR_CONFIG = "network"
# QT_TEST_SERVER_LIST = "vsftpd" "ftp-proxy" "squid" "danted"
# _REQUIREMENTS = "qtConfig(ftp)" "qtConfig(private_tests)"
