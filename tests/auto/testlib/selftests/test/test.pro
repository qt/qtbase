CONFIG += testcase
SOURCES += ../tst_selftests.cpp
QT = core testlib-private

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
!android:!winrt: for(file, SUBPROGRAMS): TEST_HELPER_INSTALLS += "../$${file}/$${file}"

