CONFIG += testcase
TARGET = tst_qtconcurrentfilter
DEFINES += QT_STRICT_ITERATORS
SOURCES += tst_qtconcurrentfilter.cpp
QT = core testlib
CONFIG += parallel_test
CONFIG += insignificant_test # See QTBUG-20688
