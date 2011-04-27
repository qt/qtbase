############################################################
# Project file for autotest for file qglbuffer.h
############################################################

load(qttest_p4)
requires(contains(QT_CONFIG,opengl))
QT += opengl

win32:!wince*: DEFINES += QT_NO_EGL

SOURCES += tst_qglbuffer.cpp
