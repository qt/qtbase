CONFIG += testcase
TARGET = tst_qhostaddress
SOURCES  += tst_qhostaddress.cpp


QT = core network testlib

win32:LIBS += -lws2_32
