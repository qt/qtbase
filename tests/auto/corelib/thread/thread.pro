TEMPLATE=subdirs

qtConfig(thread) {
    SUBDIRS=\
        qatomicint \
        qatomicinteger \
        qatomicpointer \
        qresultstore \
        qfuture \
        qfuturesynchronizer \
        qmutex \
        qmutexlocker \
        qreadlocker \
        qreadwritelock \
        qsemaphore \
        qthread \
        qthreadonce \
        qthreadpool \
        qthreadstorage \
        qwaitcondition \
        qwritelocker \
        qpromise
}

qtHaveModule(concurrent) {
    SUBDIRS += \
        qfuturewatcher
}
