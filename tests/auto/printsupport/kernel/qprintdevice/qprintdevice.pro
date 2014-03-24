CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qprintdevice
SOURCES  += tst_qprintdevice.cpp

QT += printsupport-private network testlib

DEFINES += QT_USE_USING_NAMESPACE
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
