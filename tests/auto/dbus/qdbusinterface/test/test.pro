CONFIG += testcase
SOURCES += ../tst_qdbusinterface.cpp
HEADERS += ../myobject.h
TARGET = ../tst_qdbusinterface
DESTDIR = ./

QT = core core-private dbus testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
