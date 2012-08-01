############################################################
# Project file for autotest for file qslider.h
############################################################

CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qslider
QT += widgets testlib
SOURCES += tst_qslider.cpp


DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
