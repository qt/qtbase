load(qt_module)

TARGET     = QtXml
QT         = core-private

DEFINES   += QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x61000000

load(qt_module_config)

QMAKE_DOCS = $$PWD/doc/qtxml.qdocconf
QMAKE_DOCS_INDEX = ../../doc

PRECOMPILED_HEADER = ../corelib/global/qt_pch.h

win32-borland {
        QMAKE_CFLAGS_WARN_ON        += -w-use
        QMAKE_CXXFLAGS_WARN_ON        += -w-use
}

include(dom/dom.pri)
include(sax/sax.pri)
