CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qpicture
QT += testlib
!contains(QT_CONFIG, no-widgets): QT += widgets
SOURCES  += tst_qpicture.cpp



