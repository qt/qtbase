# Generated from qnumeric.pro.

#####################################################################
## tst_qnumeric Test:
#####################################################################

qt_internal_add_test(tst_qnumeric
    SOURCES
        tst_qnumeric.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qnumeric CONDITION ICC
    COMPILE_OPTIONS
        -fp-model
        strict
)

qt_internal_extend_target(tst_qnumeric CONDITION intel_icl
    COMPILE_OPTIONS
        /fp:strict
)
