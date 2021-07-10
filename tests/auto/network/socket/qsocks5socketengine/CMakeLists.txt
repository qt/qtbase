# Generated from qsocks5socketengine.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qsocks5socketengine Test:
#####################################################################

qt_internal_add_test(tst_qsocks5socketengine
    SOURCES
        tst_qsocks5socketengine.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    QT_TEST_SERVER_LIST "danted" "apache2" "cyrus" # special case
)

#### Keys ignored in scope 1:.:.:qsocks5socketengine.pro:<TRUE>:
# MOC_DIR = "tmp"
# _REQUIREMENTS = "qtConfig(private_tests)"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qsocks5socketengine.pro:LINUX:
# QT_TEST_SERVER_LIST = "danted" "apache2" "cyrus"

qt_internal_extend_target(tst_qsocks5socketengine CONDITION WIN32
    PUBLIC_LIBRARIES
        ws2_32
)
