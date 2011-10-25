CONFIG += testcase

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES += ../tst_qapplication.cpp
TARGET = ../tst_qapplication

wince* {
  additional.files = ../desktopsettingsaware/desktopsettingsaware.exe
  additional.path = desktopsettingsaware
  someTest.files = test.pro
  someTest.path = test
  DEPLOYMENT += additional deploy someTest
}

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_qapplication
} else {
    TARGET = ../../release/tst_qapplication
  }
}

mac*:CONFIG+=insignificant_test

CONFIG += insignificant_test # QTBUG-21402
