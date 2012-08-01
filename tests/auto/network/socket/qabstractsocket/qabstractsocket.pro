############################################################
# Project file for autotest for file qabstractsocket.h
############################################################

CONFIG += testcase
TARGET = tst_qabstractsocket
QT = core network testlib

SOURCES += tst_qabstractsocket.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
