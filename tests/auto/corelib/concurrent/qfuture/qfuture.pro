CONFIG += testcase
TARGET = tst_qfuture
DEFINES += QT_STRICT_ITERATORS
SOURCES += tst_qfuture.cpp
QT = core core-private testlib
CONFIG += parallel_test
