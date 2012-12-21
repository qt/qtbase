CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qtexttable
QT += testlib
qtHaveModule(widgets): QT += widgets
SOURCES  += tst_qtexttable.cpp



DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
