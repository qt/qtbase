CONFIG += testcase parallel_test
TARGET = tst_qtconcurrentfilter
QT = core testlib concurrent
SOURCES = tst_qtconcurrentfilter.cpp
DEFINES += QT_STRICT_ITERATORS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
