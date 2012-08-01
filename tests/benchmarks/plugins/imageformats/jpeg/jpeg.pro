TEMPLATE = app
TARGET = jpeg
QT += testlib
CONFIG += release

SOURCES += jpeg.cpp

TESTDATA = n900.jpeg
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
