CONFIG += testcase
CONFIG += parallel_test

SOURCES += tst_qsslellipticcurve.cpp
!wince*:win32:LIBS += -lws2_32
QT = core network testlib

TARGET = tst_qsslellipticcurve
