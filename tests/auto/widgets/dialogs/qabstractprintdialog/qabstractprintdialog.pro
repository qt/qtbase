############################################################
# Project file for autotest for file qabstractprintdialog.h
############################################################

CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qabstractprintdialog
QT += widgets printsupport testlib
SOURCES += tst_qabstractprintdialog.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
