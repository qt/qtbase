CONFIG += testcase
TARGET = tst_qlineedit
QT += gui-private core-private widgets widgets-private testlib
SOURCES  += tst_qlineedit.cpp

osx: LIBS += -framework AppKit
