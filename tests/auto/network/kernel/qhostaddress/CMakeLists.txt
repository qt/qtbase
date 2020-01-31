# Generated from qhostaddress.pro.

#####################################################################
## tst_qhostaddress Test:
#####################################################################

add_qt_test(tst_qhostaddress
    SOURCES
        tst_qhostaddress.cpp
    LIBRARIES
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

## Scopes:
#####################################################################

extend_target(tst_qhostaddress CONDITION WIN32
    PUBLIC_LIBRARIES
        ws2_32
)
