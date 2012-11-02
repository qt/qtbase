QT = core testlib

DESTDIR = ./

win32: CONFIG += console
mac:CONFIG -= app_bundle

SOURCES += main.cpp

