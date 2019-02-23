INCLUDEPATH += ../../../../shared
HEADERS += ../../../../shared/emulationdetector.h

CONFIG += testcase
# This testcase can be slow on Windows and OS X, and may interfere with other file system tests.
win32:testcase.timeout = 900
macx:testcase.timeout = 900

QT += widgets widgets-private
QT += core-private testlib

SOURCES		+= tst_qfilesystemmodel.cpp
TARGET		= tst_qfilesystemmodel
