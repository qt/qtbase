CONFIG += testcase
TARGET = tst_qtextedit

QT += widgets widgets-private gui-private core-private testlib

SOURCES += tst_qtextedit.cpp

osx: LIBS += -framework AppKit

TESTDATA += fullWidthSelection

winrt:CONFIG += insignificant_test #QTBUG-90441
