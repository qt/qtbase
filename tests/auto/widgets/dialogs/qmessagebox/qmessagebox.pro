TEMPLATE = app
TARGET = tst_qmessagebox
QT += gui-private core-private widgets testlib
CONFIG += testcase
CONFIG += parallel_test

SOURCES += tst_qmessagebox.cpp 
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
