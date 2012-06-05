CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qtexttable
QT += testlib
!contains(QT_CONFIG, no-widgets): QT += widgets
SOURCES  += tst_qtexttable.cpp



