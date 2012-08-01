CONFIG += testcase
TARGET = tst_qgraphicsitemanimation
QT += widgets testlib
SOURCES  += tst_qgraphicsitemanimation.cpp
DEFINES += QT_NO_CAST_TO_ASCII
CONFIG += parallel_test

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
