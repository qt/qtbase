CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qmakelib
QT = core testlib

INCLUDEPATH += ../../../../qmake/library
VPATH += ../../../../qmake/library

HEADERS += \
    tst_qmakelib.h

SOURCES += \
    tst_qmakelib.cpp \
    parsertest.cpp \
    ioutils.cpp \
    proitems.cpp \
    qmakevfs.cpp \
    qmakeparser.cpp

DEFINES += PROPARSER_DEBUG
