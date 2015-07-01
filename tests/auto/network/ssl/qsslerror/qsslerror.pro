CONFIG += testcase
CONFIG += parallel_test

SOURCES += tst_qsslerror.cpp
win32:!wince: LIBS += -lws2_32
QT = core network testlib

TARGET = tst_qsslerror

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}
