CONFIG += testcase
TARGET = tst_qlistview
QT += widgets gui-private widgets-private core-private testlib
SOURCES  += tst_qlistview.cpp
win32:!wince*: LIBS += -luser32
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
