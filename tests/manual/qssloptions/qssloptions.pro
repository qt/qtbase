CONFIG += testcase
TEMPLATE = app
TARGET = tst_qssloptions
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui
QT += network testlib

#CONFIG += release

SOURCES += main.cpp
