CONFIG += testcase
CONFIG += parallel_test

TARGET = tst_qdnslookup_appless

SOURCES  += tst_qdnslookup_appless.cpp

QT = core network testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
