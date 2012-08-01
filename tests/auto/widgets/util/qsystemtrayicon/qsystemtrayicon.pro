############################################################
# Project file for autotest for file qsystemtrayicon.h
############################################################

CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qsystemtrayicon
QT += widgets testlib
SOURCES += tst_qsystemtrayicon.cpp


DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
