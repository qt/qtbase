CONFIG += testcase

QT = core-private network-private testlib
SOURCES += ../tst_qtcpsocket.cpp
win32: {
wince {
	LIBS += -lws2
} else {
	LIBS += -lws2_32
}
}

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

win32: CONFIG += insignificant_test # Hangs in release builds
