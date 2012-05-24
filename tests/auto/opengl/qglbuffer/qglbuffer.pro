############################################################
# Project file for autotest for file qglbuffer.h
############################################################

CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qglbuffer
requires(contains(QT_CONFIG,opengl))
QT += opengl widgets testlib

SOURCES += tst_qglbuffer.cpp
