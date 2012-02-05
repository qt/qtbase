TARGET     = QtConcurrent
QPRO_PWD   = $$PWD
QT         = core-private

CONFIG += module
MODULE_PRI = ../modules/qt_concurrent.pri

DEFINES   += QT_BUILD_CONCURRENT_LIB QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x66000000

unix|win32-g++*:QMAKE_PKGCONFIG_REQUIRES = QtCore

load(qt_module_config)

HEADERS += $$QT_SOURCE_TREE/src/xml/qtconcurrentversion.h

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

contains(QT_CONFIG, clock-gettime) {
    linux-*|hpux-*|solaris-*:LIBS *= -lrt
}
