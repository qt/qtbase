CONFIG += testcase
testcase.timeout = 800 # this test is slow
SOURCES  += ../tst_qudpsocket.cpp
QT = core network testlib

MOC_DIR=tmp

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = ../debug
} else {
    DESTDIR = ../release
  }
} else {
    DESTDIR = ../
}

TARGET = tst_qudpsocket
