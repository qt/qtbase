############################################################
# Project file for autotest for gui/openglconfig functionality
############################################################

CONFIG += testcase
TARGET = tst_qopenglconfig
QT += gui-private core-private testlib

SOURCES   += tst_qopenglconfig.cpp
OTHER_FILES = buglist.json
