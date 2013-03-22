############################################################
# Project file for autotest for gui/opengl functionality
############################################################

CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qopengl
QT += gui gui-private core-private testlib

SOURCES   += tst_qopengl.cpp
