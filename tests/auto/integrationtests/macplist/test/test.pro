CONFIG += testcase

QT += testlib
SOURCES += ../tst_macplist.cpp
TARGET = ../tst_macplist
win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_macplist
  } else {
    TARGET = ../../release/tst_macplist
  }
}
