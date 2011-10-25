CONFIG += testcase
TEMPLATE = app
TARGET = tst_qnetworkreply
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui
QT += network testlib

CONFIG += release

# Input
SOURCES += main.cpp
