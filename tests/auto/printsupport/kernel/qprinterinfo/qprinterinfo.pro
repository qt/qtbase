CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qprinterinfo
SOURCES  += tst_qprinterinfo.cpp

QT += printsupport network testlib

DEFINES += QT_USE_USING_NAMESPACE
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
