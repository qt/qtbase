CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qnetworkinterface
SOURCES  += tst_qnetworkinterface.cpp

QT = core network testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
