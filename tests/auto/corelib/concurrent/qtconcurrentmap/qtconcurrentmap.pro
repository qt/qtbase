CONFIG += testcase
TARGET = tst_qtconcurrentmap
DEFINES += QT_STRICT_ITERATORS
SOURCES += tst_qtconcurrentmap.cpp
QT = core testlib
CONFIG += parallel_test
