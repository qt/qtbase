# Generated from qhttpnetworkconnection.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qhttpnetworkconnection Test:
#####################################################################

add_qt_test(tst_qhttpnetworkconnection
    SOURCES
        tst_qhttpnetworkconnection.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:qhttpnetworkconnection.pro:<TRUE>:
# QT_TEST_SERVER_LIST = "apache2"
# _REQUIREMENTS = "qtConfig(private_tests)"
