CONFIG += testcase

SOURCES += tst_qsslerror.cpp
QT = core network testlib

TARGET = tst_qsslerror

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}
