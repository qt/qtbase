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
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    QT_TEST_SERVER_LIST "cyrus" # special case
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
