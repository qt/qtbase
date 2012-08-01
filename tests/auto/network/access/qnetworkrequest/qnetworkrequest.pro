CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qnetworkrequest
SOURCES  += tst_qnetworkrequest.cpp

QT = core network testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
