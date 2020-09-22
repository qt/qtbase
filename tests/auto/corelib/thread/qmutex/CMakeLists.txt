# Generated from qmutex.pro.

#####################################################################
## tst_qmutex Test:
#####################################################################

qt_internal_add_test(tst_qmutex
    SOURCES
        tst_qmutex.cpp
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmutex CONDITION WIN32
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)
