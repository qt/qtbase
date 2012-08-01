CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qxmlstream
QT = core xml network testlib
SOURCES = tst_qxmlstream.cpp

TESTDATA += data XML-Test-Suite
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
