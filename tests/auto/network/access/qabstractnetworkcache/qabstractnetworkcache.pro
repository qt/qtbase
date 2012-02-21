CONFIG += testcase
TARGET = tst_qabstractnetworkcache
QT += network testlib
QT -= gui
SOURCES  += tst_qabstractnetworkcache.cpp

TESTDATA += tests/*

CONFIG += insignificant_test  # QTBUG-20686; note, assumed unstable on all platforms
