CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qabstractnetworkcache
QT += network testlib
QT -= gui
SOURCES  += tst_qabstractnetworkcache.cpp

TESTDATA += tests/*

