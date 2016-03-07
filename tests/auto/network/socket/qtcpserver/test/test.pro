CONFIG += testcase
SOURCES  += ../tst_qtcpserver.cpp

win32:LIBS += -lws2_32

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
