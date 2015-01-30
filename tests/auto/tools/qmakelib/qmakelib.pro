CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qmakelib
QT = core testlib

INCLUDEPATH += ../../../../qmake/library
VPATH += ../../../../qmake/library

SOURCES += \
    tst_qmakelib.cpp \
    ioutils.cpp \
    proitems.cpp
