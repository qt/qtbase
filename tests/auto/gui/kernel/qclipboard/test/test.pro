CONFIG += testcase
SOURCES  += ../tst_qclipboard.cpp
TARGET = ../tst_qclipboard
QT += testlib

osx: LIBS += -framework AppKit

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_qclipboard
} else {
    TARGET = ../../release/tst_qclipboard
  }
}

wince* {
  DEPLOYMENT += rsc reg_resource
}

!winrt: TEST_HELPER_INSTALLS = \
    ../copier/copier \
    ../paster/paster
