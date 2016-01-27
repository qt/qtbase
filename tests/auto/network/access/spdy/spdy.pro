CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_spdy
SOURCES  += tst_spdy.cpp

QT = core core-private network network-private testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

win32:CONFIG += insignificant_test # QTBUG-47128
