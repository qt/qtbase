CONFIG += testcase
TARGET = tst_qitemdelegate
QT += widgets testlib
SOURCES         += tst_qitemdelegate.cpp

win32:!wince:!winrt: LIBS += -luser32
