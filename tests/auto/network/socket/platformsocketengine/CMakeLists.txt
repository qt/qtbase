# Generated from platformsocketengine.pro.

#####################################################################
## tst_platformsocketengine Test:
#####################################################################

add_qt_test(tst_platformsocketengine
    GUI
    SOURCES
        tst_platformsocketengine.cpp
    INCLUDE_DIRECTORIES
        ${PROJECT_SOURCE_DIR}/src/network
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:platformsocketengine.pro:<TRUE>:
# CONFIG = "testcase"
# MOC_DIR = "tmp"

## Scopes:
#####################################################################

extend_target(tst_platformsocketengine CONDITION WIN32
    PUBLIC_LIBRARIES
        ws2_32
)

extend_target(tst_platformsocketengine CONDITION QT_FEATURE_reduce_exports AND UNIX
    SOURCES
        ${PROJECT_SOURCE_DIR}/src/network/socket/qabstractsocketengine.cpp
        ${PROJECT_SOURCE_DIR}/src/network/socket/qnativesocketengine.cpp
        ${PROJECT_SOURCE_DIR}/src/network/socket/qnativesocketengine_unix.cpp
)
