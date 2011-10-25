CONFIG += testcase
TARGET = tst_qtemporaryfile
SOURCES       += tst_qtemporaryfile.cpp
QT = core testlib

DEFINES += SRCDIR=\\\"$$PWD/\\\"

CONFIG += parallel_test
