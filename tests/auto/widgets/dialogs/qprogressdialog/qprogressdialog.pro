############################################################
# Project file for autotest for file qprogressdialog.h
############################################################

CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qprogressdialog
QT += widgets testlib
SOURCES += tst_qprogressdialog.cpp


DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
