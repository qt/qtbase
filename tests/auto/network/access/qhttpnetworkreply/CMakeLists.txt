# Generated from qhttpnetworkreply.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qhttpnetworkreply Test:
#####################################################################

add_qt_test(tst_qhttpnetworkreply
    SOURCES
        tst_qhttpnetworkreply.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:qhttpnetworkreply.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"
