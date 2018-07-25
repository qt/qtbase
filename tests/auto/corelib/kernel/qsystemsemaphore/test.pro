CONFIG += testcase
QT = core testlib

SOURCES += tst_qsystemsemaphore.cpp
TARGET = tst_qsystemsemaphore

win32: CONFIG += console
