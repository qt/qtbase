QT = core network testlib

DESTDIR = ./
TARGET = socketprocess

win32:CONFIG += console
mac:CONFIG -= app_bundle

SOURCES += main.cpp
