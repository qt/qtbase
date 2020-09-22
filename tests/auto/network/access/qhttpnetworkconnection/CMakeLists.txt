# Generated from qhttpnetworkconnection.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qhttpnetworkconnection Test:
#####################################################################

qt_internal_add_test(tst_qhttpnetworkconnection
    SOURCES
        tst_qhttpnetworkconnection.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    QT_TEST_SERVER_LIST "apache2" # special case
)

#### Keys ignored in scope 1:.:.:qhttpnetworkconnection.pro:<TRUE>:
# QT_TEST_SERVER_LIST = "apache2"
# _REQUIREMENTS = "qtConfig(private_tests)"
