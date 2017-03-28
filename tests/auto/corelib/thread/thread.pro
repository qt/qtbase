TEMPLATE=subdirs
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
    qwritelocker

qtHaveModule(concurrent) {
    SUBDIRS += \
        qfuturewatcher
}
