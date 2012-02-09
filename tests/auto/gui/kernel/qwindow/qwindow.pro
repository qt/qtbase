CONFIG += testcase
TARGET = tst_qwindow

QT += core-private gui-private testlib

SOURCES  += tst_qwindow.cpp

mac: CONFIG += insignificant_test # QTBUG-23059
win32:CONFIG += insignificant_test # QTBUG-24185
