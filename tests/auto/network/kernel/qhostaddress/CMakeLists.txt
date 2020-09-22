# Generated from qhostaddress.pro.

#####################################################################
## tst_qhostaddress Test:
#####################################################################

qt_internal_add_test(tst_qhostaddress
    SOURCES
        tst_qhostaddress.cpp
    PUBLIC_LIBRARIES
        Qt::NetworkPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qhostaddress CONDITION WIN32
    PUBLIC_LIBRARIES
        ws2_32
)
