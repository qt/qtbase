CONFIG += testcase
TARGET = tst_qlistview
QT += widgets gui-private widgets-private core-private testlib
SOURCES  += tst_qlistview.cpp
win32:!wince*:!winrt: LIBS += -luser32
