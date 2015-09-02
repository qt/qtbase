CONFIG += testcase
TARGET = tst_spdy
SOURCES  += tst_spdy.cpp

QT = core core-private network network-private testlib

win32:CONFIG += insignificant_test # QTBUG-47128
