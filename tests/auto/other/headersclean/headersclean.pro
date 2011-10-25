CONFIG += testcase
TARGET = tst_headersclean
SOURCES  += tst_headersclean.cpp
QT = core network xml sql testlib

contains(QT_CONFIG,dbus): QT += dbus
contains(QT_CONFIG,opengl): QT += opengl
