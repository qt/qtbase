CONFIG += testcase
TARGET = tst_qsharedpointer

SOURCES += tst_qsharedpointer.cpp \
    forwarddeclaration.cpp \
    forwarddeclared.cpp \
    wrapper.cpp

HEADERS += forwarddeclared.h \
    wrapper.h

QT = core testlib
DEFINES += SRCDIR=\\\"$$PWD/\\\"

include(externaltests.pri)
CONFIG += parallel_test
