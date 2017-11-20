CONFIG += testcase
TARGET = tst_qlineedit
QT += gui-private core-private widgets widgets-private testlib testlib-private
SOURCES  += tst_qlineedit.cpp

osx: LIBS += -framework AppKit
