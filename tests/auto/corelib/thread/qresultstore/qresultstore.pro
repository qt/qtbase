CONFIG += testcase parallel_test
TARGET = tst_qresultstore
QT = core-private testlib concurrent
SOURCES = tst_qresultstore.cpp
DEFINES += QT_STRICT_ITERATORS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
