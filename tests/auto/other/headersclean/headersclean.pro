load(qttest_p4)
SOURCES  += tst_headersclean.cpp
QT = core network xml sql

contains(QT_CONFIG,dbus): QT += dbus
contains(QT_CONFIG,opengl): QT += opengl
