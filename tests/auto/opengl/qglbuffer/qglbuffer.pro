############################################################
# Project file for autotest for file qglbuffer.h
############################################################

CONFIG += testcase
TARGET = tst_qglbuffer
requires(qtHaveModule(opengl))
QT += opengl widgets testlib

SOURCES += tst_qglbuffer.cpp
