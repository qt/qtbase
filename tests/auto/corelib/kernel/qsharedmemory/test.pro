CONFIG += testcase

QT = core-private testlib

TARGET = tst_qsharedmemory
SOURCES += tst_qsharedmemory.cpp

linux: LIBS += -lrt
