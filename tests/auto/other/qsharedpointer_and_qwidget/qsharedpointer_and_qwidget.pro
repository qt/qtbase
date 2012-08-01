CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qsharedpointer_and_qwidget
QT += widgets testlib
SOURCES += tst_qsharedpointer_and_qwidget.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
