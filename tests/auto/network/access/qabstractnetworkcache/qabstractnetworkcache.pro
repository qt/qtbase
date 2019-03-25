CONFIG += testcase
TARGET = tst_qabstractnetworkcache
QT = core network testlib
SOURCES  += tst_qabstractnetworkcache.cpp

TESTDATA += tests/*

CONFIG += unsupported/testserver
QT_TEST_SERVER_LIST = apache2
