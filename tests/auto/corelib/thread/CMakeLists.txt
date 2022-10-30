# Generated from thread.pro.

if(QT_FEATURE_thread)
    add_subdirectory(qatomicint)
    add_subdirectory(qatomicinteger)
    add_subdirectory(qatomicpointer)
    add_subdirectory(qresultstore)
    if(QT_FEATURE_concurrent AND NOT INTEGRITY)
        add_subdirectory(qfuture)
    endif()
    add_subdirectory(qfuturesynchronizer)
    add_subdirectory(qmutex)
    add_subdirectory(qmutexlocker)
    add_subdirectory(qreadlocker)
    add_subdirectory(qreadwritelock)
    add_subdirectory(qsemaphore)
    # special case begin
    # QTBUG-85364
    if(NOT CMAKE_CROSSCOMPILING)
        add_subdirectory(qthread)
    endif()
    # special case end
    add_subdirectory(qthreadonce)
    add_subdirectory(qthreadpool)
    add_subdirectory(qthreadstorage)
    add_subdirectory(qwaitcondition)
    add_subdirectory(qwritelocker)
    if(NOT INTEGRITY)
        add_subdirectory(qpromise)
    endif()
endif()
# special case begin
# QTBUG-87431
if(TARGET Qt::Concurrent AND NOT INTEGRITY)
    add_subdirectory(qfuturewatcher)
endif()
# special case end
