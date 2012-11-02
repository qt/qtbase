CONFIG += testcase
QT = core testlib

win32: CONFIG += console
mac:CONFIG -= app_bundle

SOURCES += tst_qsystemsemaphore.cpp
TARGET = tst_qsystemsemaphore

DESTDIR = ../
