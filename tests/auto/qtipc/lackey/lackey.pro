include(../qsharedmemory/src/src.pri)

QT = core script

CONFIG += qtestlib

DESTDIR = ./

win32: CONFIG += console
mac:CONFIG -= app_bundle

requires(contains(QT_CONFIG,script))

DEFINES	+= QSHAREDMEMORY_DEBUG
DEFINES	+= QSYSTEMSEMAPHORE_DEBUG

SOURCES		+= main.cpp 
TARGET		= lackey


