CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qpdfwriter
SOURCES  += tst_qpdfwriter.cpp

QT += gui-private testlib

DEFINES += QT_USE_USING_NAMESPACE
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
