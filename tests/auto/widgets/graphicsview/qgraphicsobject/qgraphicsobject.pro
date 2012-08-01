CONFIG += testcase
TARGET = tst_qgraphicsobject

QT += widgets testlib
QT += core-private

SOURCES  += tst_qgraphicsobject.cpp
CONFIG += parallel_test
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
