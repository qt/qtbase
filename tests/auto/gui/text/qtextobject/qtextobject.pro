############################################################
# Project file for autotest for file qtextobject.h
############################################################

CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qtextobject
QT += testlib
!contains(QT_CONFIG, no-widgets): QT += widgets
SOURCES += tst_qtextobject.cpp


