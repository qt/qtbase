SOURCES = qpong.cpp
TARGET = qpong
DESTDIR = ./
QT = core dbus
CONFIG -= app_bundle
CONFIG += console

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
