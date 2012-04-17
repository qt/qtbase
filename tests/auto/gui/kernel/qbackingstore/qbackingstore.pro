CONFIG += testcase
TARGET = tst_qbackingstore

QT += core-private gui-private testlib

SOURCES  += tst_qbackingstore.cpp

mac: CONFIG += insignificant_test # QTBUG-23059
win32: CONFIG += insignificant_test # QTBUG-24885
