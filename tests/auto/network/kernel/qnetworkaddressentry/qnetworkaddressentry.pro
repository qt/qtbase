CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qnetworkaddressentry
SOURCES  += tst_qnetworkaddressentry.cpp

QT = core network testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
