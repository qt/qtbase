# Generated from qfreelist.pro.

#####################################################################
## tst_qfreelist Test:
#####################################################################

qt_internal_add_test(tst_qfreelist
    SOURCES
        tst_qfreelist.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qfreelist CONDITION NOT QT_FEATURE_private_tests
    SOURCES
        ${QT_SOURCE_TREE}/src/corelib/tools/qfreelist.cpp
)
