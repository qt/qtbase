CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qpagesize
SOURCES  += tst_qpagesize.cpp

QT += testlib

DEFINES += QT_USE_USING_NAMESPACE
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
