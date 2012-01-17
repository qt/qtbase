CONFIG += testcase
TARGET = tst_qsizegrip
INCLUDEPATH += .
QT += widgets testlib
SOURCES += tst_qsizegrip.cpp

mac: CONFIG += insignificant_test # failures on mac, QTBUG-23681
