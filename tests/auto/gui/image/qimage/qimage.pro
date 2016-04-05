CONFIG += testcase
TARGET = tst_qimage
SOURCES  += tst_qimage.cpp

QT += core-private gui-private testlib
contains(QT_CONFIG, c++11): CONFIG += c++11

android: RESOURCES+=qimage.qrc

TESTDATA += images/*
