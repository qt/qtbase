CONFIG += testcase

SOURCES += tst_qsslcipher.cpp
QT = core network testlib

TARGET = tst_qsslcipher

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}
