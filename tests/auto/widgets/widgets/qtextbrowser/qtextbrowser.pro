CONFIG += testcase
TARGET = tst_qtextbrowser
SOURCES += tst_qtextbrowser.cpp

QT += widgets testlib

TESTDATA += *.html *.md subdir/*

builtin_testdata: DEFINES += BUILTIN_TESTDATA
