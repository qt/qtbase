# Generated from qstatemachine.pro.

#####################################################################
## tst_qstatemachine Test:
#####################################################################

qt_add_test(tst_qstatemachine
    SOURCES
        tst_qstatemachine.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_extend_target(tst_qstatemachine CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
