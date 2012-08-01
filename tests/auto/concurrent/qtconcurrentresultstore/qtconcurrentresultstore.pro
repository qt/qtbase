CONFIG += testcase parallel_test
TARGET = tst_qtconcurrentresultstore
QT = core-private testlib concurrent
SOURCES = tst_qtconcurrentresultstore.cpp
DEFINES += QT_STRICT_ITERATORS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
