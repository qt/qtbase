CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qfont
QT += testlib
QT += core-private gui-private
qtHaveModule(widgets): QT += widgets
SOURCES  += tst_qfont.cpp


DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
