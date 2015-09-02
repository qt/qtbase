CONFIG += testcase

SOURCES += tst_qsslellipticcurve.cpp
win32:!wince: LIBS += -lws2_32
QT = core network testlib

TARGET = tst_qsslellipticcurve
