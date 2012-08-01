CONFIG += testcase
SOURCES  += ../tst_qtcpserver.cpp

win32: {
wince*: {
	LIBS += -lws2
	crashApp.files = ../crashingServer/crashingServer.exe
	crashApp.path = crashingServer
	DEPLOYMENT += crashApp
} else {
	LIBS += -lws2_32
}
}

TARGET = ../tst_qtcpserver

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_qtcpserver
} else {
    TARGET = ../../release/tst_qtcpserver
  }
}

QT = core network testlib

MOC_DIR=tmp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
