CONFIG += testcase
TARGET = tst_qtconcurrentresultstore
DEFINES += QT_STRICT_ITERATORS
SOURCES += tst_qtconcurrentresultstore.cpp
QT = core core-private testlib
CONFIG += parallel_test
