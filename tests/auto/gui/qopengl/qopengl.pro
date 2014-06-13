############################################################
# Project file for autotest for gui/opengl functionality
############################################################

CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qopengl
QT += gui-private core-private testlib

SOURCES   += tst_qopengl.cpp

win32-msvc2010:contains(QT_CONFIG, angle):CONFIG += insignificant_test # QTBUG-31611

linux:contains(QT_CONFIG, xcb-glx):contains(QT_CONFIG, xcb-xlib):!contains(QT_CONFIG, egl): DEFINES += USE_GLX
