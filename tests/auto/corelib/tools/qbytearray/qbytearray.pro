CONFIG += testcase parallel_test
TARGET = tst_qbytearray
QT = core-private testlib
SOURCES = tst_qbytearray.cpp

TESTDATA += rfc3252.txt
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
