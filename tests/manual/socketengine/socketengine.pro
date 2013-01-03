TEMPLATE = app
TARGET = tst_socketengine

QT -= gui
QT += network-private core-private testlib

SOURCES += main.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
