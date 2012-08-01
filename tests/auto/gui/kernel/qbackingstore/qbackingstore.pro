CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qbackingstore

QT += core-private gui-private testlib

SOURCES  += tst_qbackingstore.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
