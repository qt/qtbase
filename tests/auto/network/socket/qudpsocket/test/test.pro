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

wince* {
    addApp.files = ../clientserver/clientserver.exe
    addApp.path = clientserver
    DEPLOYMENT += addApp
}

TARGET = tst_qudpsocket

CONFIG+=insignificant_test # QTBUG-25367, QTBUG-25368
