CONFIG += testcase
QT = core testlib network
embedded: QT += gui
SOURCES = ../tst_qprocess.cpp

TARGET = ../tst_qprocess

TESTDATA += ../testBatFiles/*
