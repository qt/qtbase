# Generated from qhttpsocketengine.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qhttpsocketengine Test:
#####################################################################

qt_internal_add_test(tst_qhttpsocketengine
    SOURCES
        tst_qhttpsocketengine.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    QT_TEST_SERVER_LIST "squid" "danted" "cyrus" "apache2" # special case
)

#### Keys ignored in scope 1:.:.:qhttpsocketengine.pro:<TRUE>:
# MOC_DIR = "tmp"
# _REQUIREMENTS = "qtConfig(private_tests)"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qhttpsocketengine.pro:LINUX:
# QT_TEST_SERVER_LIST = "squid" "danted" "cyrus" "apache2"

qt_internal_extend_target(tst_qhttpsocketengine CONDITION WIN32
    PUBLIC_LIBRARIES
        ws2_32
)
