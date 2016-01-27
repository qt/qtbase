CONFIG += testcase
CONFIG += parallel_test
SOURCES += ../tst_selftests.cpp
QT = core xml testlib-private

TARGET = ../tst_selftests

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_selftests
} else {
    TARGET = ../../release/tst_selftests
  }
}

RESOURCES += ../selftests.qrc

include(../selftests.pri)
!winrt: for(file, SUBPROGRAMS): TEST_HELPER_INSTALLS += "../$${file}/$${file}"

