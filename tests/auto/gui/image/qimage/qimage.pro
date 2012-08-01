CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qimage
SOURCES  += tst_qimage.cpp

QT += core-private gui-private testlib

TESTDATA += images/*
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
