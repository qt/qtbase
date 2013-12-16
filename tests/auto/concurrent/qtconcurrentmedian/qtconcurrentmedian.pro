CONFIG += testcase parallel_test
TARGET = tst_qtconcurrentmedian
QT = core testlib concurrent
SOURCES = tst_qtconcurrentmedian.cpp
DEFINES += QT_STRICT_ITERATORS
