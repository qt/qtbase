TEMPLATE = app
TARGET = jpeg
QT += testlib
CONFIG += release

wince*: {
   DEFINES += SRCDIR=\\\"\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

SOURCES += jpeg.cpp
