CONFIG += testcase parallel_test
TARGET = tst_qtconcurrentmap
QT = core testlib concurrent
SOURCES = tst_qtconcurrentmap.cpp
DEFINES += QT_STRICT_ITERATORS
