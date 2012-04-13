############################################################
# Project file for autotest for file qgl.h
############################################################

CONFIG += testcase
TARGET = tst_qgl
requires(contains(QT_CONFIG,opengl))
QT += widgets widgets-private opengl-private gui-private core-private testlib

SOURCES   += tst_qgl.cpp
RESOURCES  = qgl.qrc

CONFIG+=insignificant_test # QTBUG-25293
