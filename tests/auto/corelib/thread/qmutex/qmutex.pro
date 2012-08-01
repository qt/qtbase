CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qmutex
QT = core testlib
SOURCES = tst_qmutex.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
