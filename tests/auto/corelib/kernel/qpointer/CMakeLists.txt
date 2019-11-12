# Generated from qpointer.pro.

#####################################################################
## tst_qpointer Test:
#####################################################################

add_qt_test(tst_qpointer
    SOURCES
        tst_qpointer.cpp
)

## Scopes:
#####################################################################

extend_target(tst_qpointer CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
