# Generated from qhttpsocketengine.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qhttpsocketengine Test:
#####################################################################

add_qt_test(tst_qhttpsocketengine
    SOURCES
        tst_qhttpsocketengine.cpp
    INCLUDE_DIRECTORIES
        ${QT_SOURCE_TREE}/src/network
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
)

#### Keys ignored in scope 1:.:.:qhttpsocketengine.pro:<TRUE>:
# MOC_DIR = "tmp"
# _REQUIREMENTS = "qtConfig(private_tests)"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qhttpsocketengine.pro:LINUX:
# QT_TEST_SERVER_LIST = "squid" "danted" "cyrus" "apache2"

extend_target(tst_qhttpsocketengine CONDITION WIN32
    PUBLIC_LIBRARIES
        ws2_32
)

extend_target(tst_qhttpsocketengine CONDITION QT_FEATURE_reduce_exports AND UNIX
    SOURCES
        ${QT_SOURCE_TREE}/src/network/socket/qabstractsocketengine.cpp ${QT_SOURCE_TREE}/src/network/socket/qabstractsocketengine_p.h
        ${QT_SOURCE_TREE}/src/network/socket/qnativesocketengine.cpp ${QT_SOURCE_TREE}/src/network/socket/qnativesocketengine_p.h
        ${QT_SOURCE_TREE}/src/network/socket/qnativesocketengine_unix.cpp
)
