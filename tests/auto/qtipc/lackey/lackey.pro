include(../qsharedmemory/src/src.pri)

QT = core-private script testlib

DESTDIR = ./

win32: CONFIG += console
mac:CONFIG -= app_bundle

DEFINES	+= QSHAREDMEMORY_DEBUG
DEFINES	+= QSYSTEMSEMAPHORE_DEBUG

SOURCES		+= main.cpp 
TARGET		= lackey


