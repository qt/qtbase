CONFIG += testcase
TARGET = tst_qtextmarkdownwriter
QT += core-private gui-private testlib
SOURCES += tst_qtextmarkdownwriter.cpp
TESTDATA += data/example.md

DEFINES += SRCDIR=\\\"$$PWD\\\"
