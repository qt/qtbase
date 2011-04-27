load(qttest_p4)
TEMPLATE = app
TARGET = tst_qhttpnetworkconnection
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui
QT += network

CONFIG += release

# Input
SOURCES += main.cpp
