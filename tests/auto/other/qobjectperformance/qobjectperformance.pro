CONFIG += testcase
TARGET = tst_qobjectperformance
SOURCES  += tst_qobjectperformance.cpp

QT = core network testlib


DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
