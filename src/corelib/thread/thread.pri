# Qt core thread module

# public headers
HEADERS += thread/qmutex.h \
           thread/qrunnable.h \
           thread/qreadwritelock.h \
           thread/qsemaphore.h \
           thread/qthread.h \
           thread/qthreadpool.h \
           thread/qthreadstorage.h \
           thread/qwaitcondition.h \
           thread/qatomic.h \
           thread/qatomic_bootstrap.h \
           thread/qatomic_cxx11.h \
           thread/qbasicatomic.h \
           thread/qgenericatomic.h

# private headers
HEADERS += thread/qmutex_p.h \
           thread/qmutexpool_p.h \
           thread/qfutex_p.h \
           thread/qorderedmutexlocker_p.h \
           thread/qreadwritelock_p.h \
           thread/qthread_p.h \
           thread/qthreadpool_p.h

SOURCES += thread/qatomic.cpp \
           thread/qmutex.cpp \
           thread/qreadwritelock.cpp \
           thread/qrunnable.cpp \
           thread/qmutexpool.cpp \
           thread/qsemaphore.cpp \
           thread/qthread.cpp \
           thread/qthreadpool.cpp \
           thread/qthreadstorage.cpp

qtConfig(future) {
    HEADERS += \
        thread/qexception.h \
        thread/qfuture.h \
        thread/qfutureinterface.h \
        thread/qfutureinterface_p.h \
        thread/qfuturesynchronizer.h \
        thread/qfuturewatcher.h \
        thread/qfuturewatcher_p.h \
        thread/qresultstore.h

    SOURCES += \
        thread/qexception.cpp \
        thread/qfutureinterface.cpp \
        thread/qfuturewatcher.cpp \
        thread/qresultstore.cpp
}

win32 {
    HEADERS += thread/qatomic_msvc.h

    SOURCES += \
        thread/qmutex_win.cpp \
        thread/qthread_win.cpp \
        thread/qwaitcondition_win.cpp
} else {
    darwin {
        SOURCES += thread/qmutex_mac.cpp
    } else: linux {
        SOURCES += thread/qmutex_linux.cpp
    } else {
        SOURCES += thread/qmutex_unix.cpp
    }
    SOURCES += \
        thread/qthread_unix.cpp \
        thread/qwaitcondition_unix.cpp
}

qtConfig(std-atomic64): QMAKE_USE += libatomic
