TEMPLATE	= lib
CONFIG += dll

DEFINES	+= SIMPLEDLL_MAKEDLL

HEADERS	= simple.h
SOURCES	= simple.cpp

VERSION     = 1.0.0
INCLUDEPATH += . tmp
MOC_DIR	= tmp
OBJECTS_DIR = tmp
TARGET	= "simple dll"
DESTDIR	= "dest dir"
