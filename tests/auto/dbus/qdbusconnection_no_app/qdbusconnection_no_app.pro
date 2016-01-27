CONFIG += testcase
TARGET = tst_qdbusconnection_no_app
QT = core dbus testlib
SOURCES += tst_qdbusconnection_no_app.cpp
HEADERS += ../qdbusconnection/tst_qdbusconnection.h
DEFINES += SRCDIR=\\\"$$PWD/\\\" tst_QDBusConnection=tst_QDBusConnection_NoApplication
