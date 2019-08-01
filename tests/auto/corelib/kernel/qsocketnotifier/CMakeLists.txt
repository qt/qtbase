# Generated from qsocketnotifier.pro.

#####################################################################
## tst_qsocketnotifier Test:
#####################################################################

add_qt_test(tst_qsocketnotifier
    GUI
    SOURCES
        tst_qsocketnotifier.cpp
    INCLUDE_DIRECTORIES
        ${QT_SOURCE_TREE}/src/network
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:qsocketnotifier.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

extend_target(tst_qsocketnotifier CONDITION WIN32
    PUBLIC_LIBRARIES
        ws2_32
)

extend_target(tst_qsocketnotifier CONDITION QT_FEATURE_reduce_exports AND UNIX
    SOURCES
        ${QT_SOURCE_TREE}/src/network/socket/qabstractsocketengine.cpp
        ${QT_SOURCE_TREE}/src/network/socket/qnativesocketengine.cpp
        ${QT_SOURCE_TREE}/src/network/socket/qnativesocketengine_unix.cpp
)
