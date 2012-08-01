CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qnetworkcachemetadata
QT -= gui
QT += network testlib
SOURCES  += tst_qnetworkcachemetadata.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
