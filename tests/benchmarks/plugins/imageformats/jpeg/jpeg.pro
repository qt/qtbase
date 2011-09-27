load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = jpeg
DEPENDPATH += .
INCLUDEPATH += .

CONFIG += release

wince*: {
   DEFINES += SRCDIR=\\\"\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

# Input
SOURCES += jpeg.cpp
