CONFIG += testcase
TARGET = tst_qtextbrowser
SOURCES += tst_qtextbrowser.cpp

QT += widgets testlib

TESTDATA += *.html subdir/*
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
