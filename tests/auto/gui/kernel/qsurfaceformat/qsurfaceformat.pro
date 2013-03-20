CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qsurfaceformat

QT += core-private gui-private testlib

SOURCES  += tst_qsurfaceformat.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
