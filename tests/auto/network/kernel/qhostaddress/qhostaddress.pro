CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qhostaddress
SOURCES  += tst_qhostaddress.cpp


QT = core network testlib

win32: {
wince*: {
	LIBS += -lws2
} else {
	LIBS += -lws2_32
}
}
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
