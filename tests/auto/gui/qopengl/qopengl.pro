############################################################
# Project file for autotest for gui/opengl functionality
############################################################

CONFIG += testcase
TARGET = tst_qopengl
QT += gui gui-private core-private testlib

SOURCES   += tst_qopengl.cpp

mac: CONFIG += insignificant_test # QTBUG-23061
win32:CONFIG += insignificant_test # QTBUG-24192
