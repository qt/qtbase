CONFIG += testcase
TARGET = tst_qtextbrowser
SOURCES += tst_qtextbrowser.cpp

QT += widgets testlib

TESTDATA += *.html subdir/*

builtin_testdata: DEFINES += BUILTIN_TESTDATA
