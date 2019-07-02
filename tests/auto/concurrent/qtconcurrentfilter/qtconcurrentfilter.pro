CONFIG += testcase
TARGET = tst_qtconcurrentfilter
QT = core testlib concurrent
SOURCES = tst_qtconcurrentfilter.cpp
DEFINES += QT_STRICT_ITERATORS
DEFINES -= QT_NO_LINKED_LIST
