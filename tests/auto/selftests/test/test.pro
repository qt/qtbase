load(qttest_p4)
SOURCES += ../tst_selftests.cpp
QT += core xml

TARGET = ../tst_selftests

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_selftests
} else {
    TARGET = ../../release/tst_selftests
  }
}

RESOURCES += ../selftests.qrc


