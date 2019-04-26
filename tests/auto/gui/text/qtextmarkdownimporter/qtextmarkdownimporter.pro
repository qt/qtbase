CONFIG += testcase
TARGET = tst_qtextmarkdownimporter
QT += core-private gui-private testlib
SOURCES += tst_qtextmarkdownimporter.cpp
TESTDATA += data/headingBulletsContinuations.md

DEFINES += SRCDIR=\\\"$$PWD\\\"
