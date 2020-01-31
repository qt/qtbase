# Generated from qtconcurrentfilter.pro.

#####################################################################
## tst_qtconcurrentfilter Test:
#####################################################################

add_qt_test(tst_qtconcurrentfilter
    SOURCES
        tst_qtconcurrentfilter.cpp
    DEFINES
        # -QT_NO_LINKED_LIST # special case remove until figured out how to deal with this
        QT_STRICT_ITERATORS
    PUBLIC_LIBRARIES
        Qt::Concurrent
)
