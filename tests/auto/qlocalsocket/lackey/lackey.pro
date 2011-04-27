#include(../src/src.pri)

QT = core script network

requires(contains(QT_CONFIG,script))

CONFIG += qtestlib

DESTDIR = ./

win32: CONFIG += console
mac:CONFIG -= app_bundle

DEFINES += QLOCALSERVER_DEBUG
DEFINES += QLOCALSOCKET_DEBUG

SOURCES		+= main.cpp
TARGET		= lackey

symbian:TARGET.CAPABILITY = ALL -TCB