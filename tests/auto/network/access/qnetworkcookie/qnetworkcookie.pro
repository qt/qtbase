CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qnetworkcookie
SOURCES  += tst_qnetworkcookie.cpp

QT = core network testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
