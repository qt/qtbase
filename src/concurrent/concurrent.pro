load(qt_build_config)

TARGET     = QtConcurrent
QT         = core-private
CONFIG    += exceptions

DEFINES   += QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x66000000

load(qt_module)

PRECOMPILED_HEADER = ../corelib/global/qt_pch.h

SOURCES += \
        qfuture.cpp \
        qfutureinterface.cpp \
        qfuturesynchronizer.cpp \
        qfuturewatcher.cpp \
        qtconcurrentfilter.cpp \
        qtconcurrentmap.cpp \
        qtconcurrentresultstore.cpp \
        qtconcurrentthreadengine.cpp \
        qtconcurrentiteratekernel.cpp \
        qtconcurrentexception.cpp

HEADERS += \
        qtconcurrent_global.h \
        qfuture.h \
        qfutureinterface.h \
        qfuturesynchronizer.h \
        qfuturewatcher.h \
        qtconcurrentcompilertest.h \
        qtconcurrentexception.h \
        qtconcurrentfilter.h \
        qtconcurrentfilterkernel.h \
        qtconcurrentfunctionwrappers.h \
        qtconcurrentiteratekernel.h \
        qtconcurrentmap.h \
        qtconcurrentmapkernel.h \
        qtconcurrentmedian.h \
        qtconcurrentreducekernel.h \
        qtconcurrentresultstore.h \
        qtconcurrentrun.h \
        qtconcurrentrunbase.h \
        qtconcurrentstoredfunctioncall.h \
        qtconcurrentthreadengine.h

# private headers
HEADERS += \
        qfutureinterface_p.h \
        qfuturewatcher_p.h

QMAKE_DOCS = $$PWD/doc/qtconcurrent.qdocconf
QMAKE_DOCS_INDEX = ../../doc

contains(QT_CONFIG, clock-gettime) {
    linux-*|hpux-*|solaris-*:LIBS *= -lrt
}
