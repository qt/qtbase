CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qnetworkcookiejar
SOURCES  += tst_qnetworkcookiejar.cpp

QT = core core-private network network-private testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
