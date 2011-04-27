load(qttest_p4)
TEMPLATE = app
TARGET = tst_socketengine
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui
QT += network

CONFIG += release

symbian: TARGET.CAPABILITY = NetworkServices

# Input
SOURCES += main.cpp
