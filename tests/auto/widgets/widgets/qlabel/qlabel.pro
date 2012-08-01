CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qlabel

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES += tst_qlabel.cpp

TESTDATA += testdata/* *.png
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
