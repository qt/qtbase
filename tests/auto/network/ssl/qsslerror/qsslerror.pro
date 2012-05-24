CONFIG += testcase
CONFIG += parallel_test

SOURCES += tst_qsslerror.cpp
!wince*:win32:LIBS += -lws2_32
QT = core network testlib

TARGET = tst_qsslerror

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}
