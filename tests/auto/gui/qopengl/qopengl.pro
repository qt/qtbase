############################################################
# Project file for autotest for gui/opengl functionality
############################################################

CONFIG += testcase
TARGET = tst_qopengl
QT += gui-private core-private testlib

SOURCES   += tst_qopengl.cpp

linux:qtConfig(xcb):qtConfig(xcb-glx-plugin): DEFINES += USE_GLX
