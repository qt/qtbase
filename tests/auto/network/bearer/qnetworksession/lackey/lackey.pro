SOURCES += main.cpp
TARGET = lackey

QT = core network

DESTDIR = ./

win32:CONFIG += console
mac:CONFIG -= app_bundle
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
