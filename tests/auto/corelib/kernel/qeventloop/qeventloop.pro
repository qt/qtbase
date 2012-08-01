CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qeventloop
QT = core network testlib core-private
SOURCES = tst_qeventloop.cpp

win32:!wince*:LIBS += -luser32
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
