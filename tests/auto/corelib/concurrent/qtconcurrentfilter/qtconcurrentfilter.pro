CONFIG += testcase parallel_test
TARGET = tst_qtconcurrentfilter
QT = core testlib
SOURCES = tst_qtconcurrentfilter.cpp
DEFINES += QT_STRICT_ITERATORS

CONFIG += insignificant_test # See QTBUG-20688
