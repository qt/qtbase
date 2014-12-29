SOURCES = qmyserver.cpp
HEADERS = ../myobject.h
TARGET = qmyserver
DESTDIR = ./
QT = core dbus
CONFIG -= app_bundle
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
