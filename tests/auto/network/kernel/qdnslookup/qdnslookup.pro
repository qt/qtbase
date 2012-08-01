CONFIG += testcase
CONFIG += parallel_test

TARGET = tst_qdnslookup

SOURCES  += tst_qdnslookup.cpp

QT = core network testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
