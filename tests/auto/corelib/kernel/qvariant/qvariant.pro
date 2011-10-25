CONFIG += testcase
TARGET = tst_qvariant
SOURCES  += tst_qvariant.cpp
QT += widgets network testlib

CONFIG+=insignificant_test # See QTBUG-8959
