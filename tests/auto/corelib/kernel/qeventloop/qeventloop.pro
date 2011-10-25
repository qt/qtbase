CONFIG += testcase
TARGET = tst_qeventloop
SOURCES += tst_qeventloop.cpp
QT -= gui 
QT += network testlib

win32:!wince*:LIBS += -luser32
