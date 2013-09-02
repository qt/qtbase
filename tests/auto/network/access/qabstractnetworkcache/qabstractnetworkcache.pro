CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qabstractnetworkcache
QT = core network testlib
SOURCES  += tst_qabstractnetworkcache.cpp

TESTDATA += tests/*

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
