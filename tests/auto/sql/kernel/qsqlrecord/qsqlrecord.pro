CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qsqlrecord
SOURCES  += tst_qsqlrecord.cpp

QT = core sql testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
