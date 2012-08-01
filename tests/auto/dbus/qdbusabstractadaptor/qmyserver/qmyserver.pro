SOURCES = qmyserver.cpp
HEADERS = ../myobject.h
TARGET = qmyserver
QT += dbus
QT -= gui
CONFIG -= app_bundle
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
