# Generated from qtconcurrentrun.pro.

#####################################################################
## tst_qtconcurrentrun Test:
#####################################################################

add_qt_test(tst_qtconcurrentrun
    SOURCES
        tst_qtconcurrentrun.cpp
    LIBRARIES
        Qt::Concurrent
)

## Scopes:
#####################################################################
#
#extend_target(tst_qtconcurrentrun CONDITION contains(QT_CONFIG, c++1z)
#)
