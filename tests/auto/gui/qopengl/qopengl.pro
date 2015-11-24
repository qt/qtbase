############################################################
# Project file for autotest for gui/opengl functionality
############################################################

CONFIG += testcase
TARGET = tst_qopengl
QT += gui-private core-private testlib

SOURCES   += tst_qopengl.cpp

linux:contains(QT_CONFIG, xcb-glx):contains(QT_CONFIG, xcb-xlib):!contains(QT_CONFIG, egl): DEFINES += USE_GLX
