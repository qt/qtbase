CONFIG += testcase
TARGET = tst_qmutex
QT = core testlib
SOURCES = tst_qmutex.cpp
win32:QT += core-private
