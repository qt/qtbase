CONFIG += testcase parallel_test
TARGET = tst_qfuture
QT = core core-private testlib concurrent
SOURCES = tst_qfuture.cpp
DEFINES += QT_STRICT_ITERATORS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
