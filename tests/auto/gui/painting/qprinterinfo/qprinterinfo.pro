CONFIG += testcase
TARGET = tst_qprinterinfo
SOURCES  += tst_qprinterinfo.cpp

QT += printsupport network testlib

DEFINES += QT_USE_USING_NAMESPACE

CONFIG += insignificant_test # QTBUG-21402
