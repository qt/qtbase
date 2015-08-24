CONFIG += testcase
SOURCES += ../tst_qdbusabstractinterface.cpp ../interface.cpp
HEADERS += ../interface.h

TARGET = ../tst_qdbusabstractinterface
DESTDIR = ./

QT = core testlib
QT += dbus

DBUS_INTERFACES = ../org.qtproject.QtDBus.Pinger.xml
QDBUSXML2CPP_INTERFACE_HEADER_FLAGS += -i ../interface.h
