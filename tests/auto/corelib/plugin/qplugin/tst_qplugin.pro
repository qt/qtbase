CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qplugin
QT = core testlib
SOURCES = tst_qplugin.cpp

TESTDATA += plugins/*
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
