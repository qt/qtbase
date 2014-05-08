SOURCES = qpinger.cpp ../interface.cpp
HEADERS = ../interface.h
TARGET = qpinger
CONFIG -= app_bundle
CONFIG += console
QT = core dbus
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
