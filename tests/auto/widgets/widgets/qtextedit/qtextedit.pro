CONFIG += testcase
TARGET = tst_qtextedit

QT += widgets widgets-private gui-private core-private testlib

SOURCES += tst_qtextedit.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
