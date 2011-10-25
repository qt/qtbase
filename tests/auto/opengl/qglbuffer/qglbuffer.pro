############################################################
# Project file for autotest for file qglbuffer.h
############################################################

CONFIG += testcase
TARGET = tst_qglbuffer
requires(contains(QT_CONFIG,opengl))
QT += opengl widgets testlib

win32:!wince*: DEFINES += QT_NO_EGL

SOURCES += tst_qglbuffer.cpp
