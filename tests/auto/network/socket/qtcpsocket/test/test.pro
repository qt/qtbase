CONFIG += testcase

QT = core-private network-private testlib
SOURCES += ../tst_qtcpsocket.cpp
win32:LIBS += -lws2_32

TARGET = tst_qtcpsocket

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = ../debug
} else {
    DESTDIR = ../release
  }
} else {
    DESTDIR = ../
}
