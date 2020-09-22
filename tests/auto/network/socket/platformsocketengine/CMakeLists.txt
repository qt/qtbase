# Generated from platformsocketengine.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_platformsocketengine Test:
#####################################################################

qt_internal_add_test(tst_platformsocketengine
    SOURCES
        tst_platformsocketengine.cpp
    INCLUDE_DIRECTORIES
        ${QT_SOURCE_TREE}/src/network
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
)

#### Keys ignored in scope 1:.:.:platformsocketengine.pro:<TRUE>:
# MOC_DIR = "tmp"
# _REQUIREMENTS = "qtConfig(private_tests)"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_platformsocketengine CONDITION WIN32
    PUBLIC_LIBRARIES
        ws2_32
)
