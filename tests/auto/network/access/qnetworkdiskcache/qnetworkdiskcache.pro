CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qnetworkdiskcache
QT -= gui
QT += network testlib
SOURCES  += tst_qnetworkdiskcache.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
