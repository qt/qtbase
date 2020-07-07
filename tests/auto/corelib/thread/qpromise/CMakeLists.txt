# Generated from qpromise.pro.

#####################################################################
## tst_qpromise Test:
#####################################################################

qt_add_test(tst_qpromise
    SOURCES
        tst_qpromise.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
    LIBRARIES # special case
        Threads::Threads  # solves issue with libpthread linkage # special case
)
