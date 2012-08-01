TARGET = tst_bench_qdiriterator

QT = core testlib

CONFIG += release

SOURCES += main.cpp qfilesystemiterator.cpp
HEADERS += qfilesystemiterator.h

wince* {
   corelibdir.files = $$QT_SOURCE_TREE/src/corelib
   corelibdir.path = ./depot/src
   DEPLOYMENT += corelibdir
}

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
