load(qttest_p4)
TEMPLATE = app
TARGET = tst_qssloptions
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui
QT += network

#CONFIG += release

SOURCES += main.cpp
