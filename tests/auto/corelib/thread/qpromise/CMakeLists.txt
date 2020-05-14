#####################################################################
## tst_qpromise Test:
#####################################################################

qt_add_test(tst_qpromise
    SOURCES
        tst_qpromise.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
    LIBRARIES
        Threads::Threads  # solves issue with libpthread linkage
)
