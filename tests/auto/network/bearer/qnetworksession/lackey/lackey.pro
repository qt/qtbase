SOURCES += main.cpp
TARGET = lackey

QT = core network

DESTDIR = ./

win32:CONFIG += console
mac:CONFIG -= app_bundle
