CONFIG += testcase
CONFIG -= app_bundle debug_and_release_target

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES += ../tst_qapplication.cpp
TARGET = ../tst_qapplication

TESTDATA = ../test/test.pro ../tmp/README

!winrt {
  SUBPROGRAMS = desktopsettingsaware modal
  win32:SUBPROGRAMS += wincmdline

  for(file, SUBPROGRAMS): TEST_HELPER_INSTALLS += "../$${file}/$${file}"
}

