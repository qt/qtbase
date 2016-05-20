CONFIG += testcase
TARGET = tst_qdbusconnection_spyhook
QT = core dbus testlib
SOURCES += tst_qdbusconnection_spyhook.cpp
HEADERS += ../qdbusconnection/tst_qdbusconnection.h
DEFINES += SRCDIR=\\\"$$PWD/\\\" tst_QDBusConnection=tst_QDBusConnection_SpyHook
include(../dbus-testcase.pri)
