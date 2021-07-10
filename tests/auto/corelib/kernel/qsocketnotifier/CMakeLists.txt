# Generated from qsocketnotifier.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qsocketnotifier Test:
#####################################################################

qt_internal_add_test(tst_qsocketnotifier
    SOURCES
        tst_qsocketnotifier.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::NetworkPrivate
)

#### Keys ignored in scope 1:.:.:qsocketnotifier.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qsocketnotifier CONDITION WIN32
    PUBLIC_LIBRARIES
        ws2_32
)
