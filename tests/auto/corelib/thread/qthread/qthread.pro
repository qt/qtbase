CONFIG += testcase
TARGET = tst_qthread
QT = core testlib
SOURCES = tst_qthread.cpp
qtConfig(c++14):CONFIG += c++14
qtConfig(c++1z):CONFIG += c++1z

INCLUDEPATH += ../../../../shared/
HEADERS += ../../../../shared/emulationdetector.h
