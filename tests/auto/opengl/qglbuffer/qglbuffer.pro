############################################################
# Project file for autotest for file qglbuffer.h
############################################################

CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qglbuffer
requires(qtHaveModule(opengl))
QT += opengl widgets testlib

SOURCES += tst_qglbuffer.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
