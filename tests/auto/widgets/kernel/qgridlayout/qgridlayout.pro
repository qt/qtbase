CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qgridlayout

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES  += tst_qgridlayout.cpp
FORMS           += sortdialog.ui



DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
