load(qttest_p4)
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

symbian {
    crashApp.files = $$QT_BUILD_TREE/examples/widgets/wiggly/$${BUILD_DIR}/crashingServer.exe
    crashApp.path = .
    DEPLOYMENT += crashApp
    TARGET.CAPABILITY += NetworkServices ReadUserData
}

TARGET = ../tst_qtcpserver

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_qtcpserver
} else {
    TARGET = ../../release/tst_qtcpserver
  }
}

QT = core network

MOC_DIR=tmp

CONFIG += insignificant_test # QTBUG-21402
