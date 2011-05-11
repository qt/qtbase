load(qttest_p4)

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

symbian: {
  additional.files = $$OUT_PWD/../desktopsettingsaware/desktopsettingsaware.exe
  additional.path = desktopsettingsaware
  someTest.files = test.pro
  someTest.path = test
  windowIcon.files = ../heart.svg
  DEPLOYMENT += additional deploy someTest windowIcon
  LIBS += -lcone -lavkon
}

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_qapplication
} else {
    TARGET = ../../release/tst_qapplication
  }
}


