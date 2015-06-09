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
    evaltest.cpp \
    ioutils.cpp \
    proitems.cpp \
    qmakevfs.cpp \
    qmakeparser.cpp \
    qmakeglobals.cpp \
    qmakebuiltins.cpp \
    qmakeevaluator.cpp

DEFINES += PROPARSER_DEBUG PROEVALUATOR_FULL PROEVALUATOR_SETENV
