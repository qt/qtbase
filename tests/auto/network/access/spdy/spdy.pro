CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_spdy
SOURCES  += tst_spdy.cpp

QT = core core-private network network-private testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
