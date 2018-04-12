############################################################
# Project file for autotest for file qfiledialog.h
############################################################

CONFIG += testcase
TARGET = tst_qfiledialog
QT += widgets widgets-private testlib
QT += core-private gui-private
SOURCES += tst_qfiledialog.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"
