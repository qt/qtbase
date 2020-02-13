# Generated from qnumeric.pro.

#####################################################################
## tst_qnumeric Test:
#####################################################################

add_qt_test(tst_qnumeric
    SOURCES
        tst_qnumeric.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

extend_target(tst_qnumeric CONDITION ICC
    COMPILE_OPTIONS
        -fp-model
        strict
)

extend_target(tst_qnumeric CONDITION intel_icl
    COMPILE_OPTIONS
        /fp:strict
)
