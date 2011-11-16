CONFIG += testcase
TARGET = tst_qvariant
QT = widgets network testlib
SOURCES = tst_qvariant.cpp

mac: CONFIG += insignificant_test # QTBUG-QTBUG-22747
