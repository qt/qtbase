# Generated from qtconcurrentmap.pro.

#####################################################################
## tst_qtconcurrentmap Test:
#####################################################################

add_qt_test(tst_qtconcurrentmap
    SOURCES
        tst_qtconcurrentmap.cpp
    DEFINES
        # -QT_NO_LINKED_LIST # special case remove until we figure this out
        QT_STRICT_ITERATORS
    PUBLIC_LIBRARIES
        Qt::Concurrent
)

## Scopes:
#####################################################################
