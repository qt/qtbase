CONFIG += testcase

QT += widgets testlib
QT += core-private network-private
SOURCES += ../tst_qtcpsocket.cpp
win32: {
wince*: {
	LIBS += -lws2
} else {
	LIBS += -lws2_32
}
}
QT += network
vxworks:QT -= gui

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

CONFIG+=insignificant_test  # unstable, QTBUG-21043
