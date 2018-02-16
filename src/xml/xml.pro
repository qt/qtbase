TARGET     = QtXml
QT         = core-private

DEFINES   += QT_NO_USING_NAMESPACE QT_NO_FOREACH

msvc:equals(QT_ARCH, i386): QMAKE_LFLAGS += /BASE:0x61000000

QMAKE_DOCS = $$PWD/doc/qtxml.qdocconf

HEADERS += qtxmlglobal.h

PRECOMPILED_HEADER =

include(dom/dom.pri)
include(sax/sax.pri)

load(qt_module)
