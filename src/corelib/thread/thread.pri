# Qt core thread module

HEADERS += \
    thread/qmutex.h \
    thread/qreadwritelock.h \
    thread/qrunnable.h \
    thread/qthread.h \
    thread/qthreadstorage.h \
    thread/qwaitcondition_p.h \
    thread/qwaitcondition.h

SOURCES += \
    thread/qrunnable.cpp \
    thread/qthread.cpp

win32 {
    HEADERS += thread/qatomic_msvc.h

    SOURCES += thread/qthread_win.cpp
} else {
    SOURCES += thread/qthread_unix.cpp
}

qtConfig(thread) {
    HEADERS += \
        thread/qatomic.h \
        thread/qatomic_bootstrap.h \
        thread/qatomic_cxx11.h \
        thread/qbasicatomic.h \
        thread/qfutex_p.h \
        thread/qgenericatomic.h \
        thread/qlocking_p.h \
        thread/qmutex_p.h \
        thread/qorderedmutexlocker_p.h \
        thread/qreadwritelock_p.h \
        thread/qsemaphore.h \
        thread/qthreadpool.h \
        thread/qthreadpool_p.h \
        thread/qthread_p.h

    SOURCES += \
       thread/qatomic.cpp \
       thread/qmutex.cpp \
       thread/qreadwritelock.cpp \
       thread/qsemaphore.cpp \
       thread/qthreadpool.cpp \
       thread/qthreadstorage.cpp

    win32 {
        SOURCES += \
            thread/qmutex_win.cpp \
            thread/qwaitcondition_win.cpp
    } else {
        darwin {
            SOURCES += thread/qmutex_mac.cpp
        } else: linux {
            SOURCES += thread/qmutex_linux.cpp
        } else {
            SOURCES += thread/qmutex_unix.cpp
        }
        SOURCES += thread/qwaitcondition_unix.cpp
    }
}

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

qtConfig(std-atomic64): QMAKE_USE += libatomic
