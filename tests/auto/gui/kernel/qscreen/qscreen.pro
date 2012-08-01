CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qscreen

QT += core-private gui-private testlib

SOURCES  += tst_qscreen.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
