TEMPLATE	= lib
CONFIG	+= qt warn_on dll

win32:DEFINES	+= SIMPLEDLL_MAKEDLL

HEADERS	= simple.h
SOURCES	= simple.cpp

VERSION     = 1.0.0
INCLUDEPATH += . tmp
MOC_DIR	= tmp
OBJECTS_DIR = tmp
TARGET	= simple_dll
DESTDIR	= ./

infile($(QTDIR)/.qmake.cache, CONFIG, debug):CONFIG += debug
infile($(QTDIR)/.qmake.cache, CONFIG, release):CONFIG += release


