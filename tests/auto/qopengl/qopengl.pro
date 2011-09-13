############################################################
# Project file for autotest for gui/opengl functionality
############################################################

load(qttest_p4)
QT += gui gui-private core-private

SOURCES   += tst_qopengl.cpp

CONFIG += insignificant_test
