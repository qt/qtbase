CONFIG += testcase
TARGET = tst_qlistview
QT += widgets gui-private testlib
SOURCES  += tst_qlistview.cpp
win32:!wince*: LIBS += -luser32
