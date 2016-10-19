CONFIG   += testcase
QT       += testlib core-private

QT       -= gui

TARGET = tst_qglobalstatic
CONFIG   += console
CONFIG   += exceptions

SOURCES += tst_qglobalstatic.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
