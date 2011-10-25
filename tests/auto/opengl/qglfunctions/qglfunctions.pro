CONFIG += testcase
TARGET = tst_qglfunctions
requires(contains(QT_CONFIG,opengl))
QT += opengl widgets testlib

win32:!wince*: DEFINES += QT_NO_EGL

SOURCES += tst_qglfunctions.cpp
