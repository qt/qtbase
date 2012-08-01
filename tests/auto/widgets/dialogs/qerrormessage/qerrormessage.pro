CONFIG += testcase
CONFIG += parallel_test
TEMPLATE = app
TARGET = tst_qerrormessage

QT += widgets testlib

SOURCES += tst_qerrormessage.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
