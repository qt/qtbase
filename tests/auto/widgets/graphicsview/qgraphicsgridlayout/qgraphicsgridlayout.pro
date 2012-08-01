CONFIG += testcase
TARGET = tst_qgraphicsgridlayout

QT += widgets testlib
SOURCES  += tst_qgraphicsgridlayout.cpp
CONFIG += parallel_test
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
