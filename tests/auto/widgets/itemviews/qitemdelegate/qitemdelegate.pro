CONFIG += testcase
TARGET = tst_qitemdelegate
QT += widgets testlib
SOURCES         += tst_qitemdelegate.cpp

win32:!wince*: LIBS += -luser32
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
