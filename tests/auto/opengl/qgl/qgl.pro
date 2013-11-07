############################################################
# Project file for autotest for file qgl.h
############################################################

CONFIG += testcase
TARGET = tst_qgl
requires(qtHaveModule(opengl))
QT += widgets widgets-private opengl-private gui-private core-private testlib

SOURCES   += tst_qgl.cpp
RESOURCES  = qgl.qrc

linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = lucid ]"): CONFIG+=insignificant_test # QTBUG-25293
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
win32-msvc2010:contains(QT_CONFIG, angle):CONFIG += insignificant_test # QTQAINFRA-711
