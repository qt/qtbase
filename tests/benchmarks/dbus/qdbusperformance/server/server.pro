SOURCES = server.cpp
HEADERS = ../serverobject.h
TARGET = server
DESTDIR = .
QT += dbus
QT -= gui
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
