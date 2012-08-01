SOURCES = qpinger.cpp ../interface.cpp
HEADERS = ../interface.h
TARGET = qpinger
QT += dbus
QT -= gui
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
