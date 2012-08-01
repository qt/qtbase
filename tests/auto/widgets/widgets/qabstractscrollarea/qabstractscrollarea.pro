############################################################
# Project file for autotest for file qabstractscrollarea.h
############################################################

CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qabstractscrollarea
QT += widgets testlib
SOURCES += tst_qabstractscrollarea.cpp


DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
