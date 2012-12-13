############################################################
# Project file for autotest for file qabstracttextdocumentlayout.h
############################################################

CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qabstracttextdocumentlayout
QT += testlib
SOURCES += tst_qabstracttextdocumentlayout.cpp
linux: CONFIG += insignificant_test

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
