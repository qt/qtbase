CONFIG += testcase

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES += ../tst_qapplication.cpp
TARGET = ../tst_qapplication

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_qapplication
} else {
    TARGET = ../../release/tst_qapplication
  }
}

TESTDATA = ../test/test.pro ../tmp/README

SUBPROGRAMS = desktopsettingsaware modal
win32: !wince*: SUBPROGRAMS += wincmdline

load(testcase) # for target.path and installTestHelperApp()
for(file, SUBPROGRAMS): installTestHelperApp("../$${file}/$${file}",$${file},$${file})


