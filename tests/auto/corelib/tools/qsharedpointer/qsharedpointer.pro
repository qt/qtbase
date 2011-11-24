CONFIG += testcase parallel_test
TARGET = tst_qsharedpointer
QT = core testlib

SOURCES = tst_qsharedpointer.cpp \
    forwarddeclaration.cpp \
    forwarddeclared.cpp \
    wrapper.cpp

HEADERS = forwarddeclared.h \
    wrapper.h

TESTDATA += forwarddeclared.cpp forwarddeclared.h

include(externaltests.pri)
