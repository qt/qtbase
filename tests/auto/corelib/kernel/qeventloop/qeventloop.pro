CONFIG += testcase
TARGET = tst_qeventloop
QT = network testlib
SOURCES = tst_qeventloop.cpp

win32:!wince*:LIBS += -luser32
