CONFIG += testcase
TEMPLATE = app
TARGET = tst_qnetworkconfigurationmanager
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui
QT += network testlib

CONFIG += release

# Input
SOURCES += main.cpp
