CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qcolordialog
QT += widgets testlib
SOURCES  += tst_qcolordialog.cpp

linux*: CONFIG += insignificant_test # Crashes on different Linux distros
