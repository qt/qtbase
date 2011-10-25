TEMPLATE = app
TARGET = jpeg
DEPENDPATH += .
INCLUDEPATH += .
QT += testlib
CONFIG += release

wince*: {
   DEFINES += SRCDIR=\\\"\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

# Input
SOURCES += jpeg.cpp
