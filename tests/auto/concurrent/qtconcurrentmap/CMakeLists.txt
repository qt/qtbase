# Generated from qtconcurrentmap.pro.

#####################################################################
## tst_qtconcurrentmap Test:
#####################################################################

add_qt_test(tst_qtconcurrentmap
    SOURCES
        tst_qtconcurrentmap.cpp
    DEFINES
        QT_STRICT_ITERATORS
    LIBRARIES
        Qt::Concurrent
)

## Scopes:
#####################################################################
#
#extend_target(tst_qtconcurrentmap CONDITION contains(QT_CONFIG, c++1z)
#)
