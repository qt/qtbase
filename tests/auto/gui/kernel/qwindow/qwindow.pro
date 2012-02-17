CONFIG += testcase
TARGET = tst_qwindow

QT += core-private gui-private testlib

SOURCES  += tst_qwindow.cpp

mac: CONFIG += insignificant_test # QTBUG-23059

