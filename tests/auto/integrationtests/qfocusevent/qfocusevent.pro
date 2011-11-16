CONFIG += testcase
TARGET = tst_qfocusevent
QT += widgets testlib
SOURCES += tst_qfocusevent.cpp
mac: CONFIG += insignificant_test # QTBUG-22815
