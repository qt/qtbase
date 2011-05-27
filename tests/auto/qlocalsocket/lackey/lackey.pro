#include(../src/src.pri)

QT = core script network testlib

DESTDIR = ./

win32: CONFIG += console
mac:CONFIG -= app_bundle

DEFINES += QLOCALSERVER_DEBUG
DEFINES += QLOCALSOCKET_DEBUG

SOURCES		+= main.cpp
TARGET		= lackey

symbian:TARGET.CAPABILITY = ALL -TCB
