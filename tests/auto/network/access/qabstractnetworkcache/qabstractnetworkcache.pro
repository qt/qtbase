CONFIG += testcase
TARGET = tst_qabstractnetworkcache
QT = core network testlib
SOURCES  += tst_qabstractnetworkcache.cpp

TESTDATA += tests/*

QT_TEST_SERVER_LIST = apache2
include($$dirname(_QMAKE_CONF_)/tests/auto/testserver.pri)
