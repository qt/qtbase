CONFIG += testcase
TARGET = tst_qscreen

QT += core-private gui-private testlib

SOURCES  += tst_qscreen.cpp

CONFIG += insignificant_test # QTBUG-22554
