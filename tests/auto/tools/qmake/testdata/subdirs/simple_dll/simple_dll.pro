TEMPLATE	= lib
CONFIG += dll

DEFINES	+= SIMPLEDLL_MAKEDLL

HEADERS	= simple.h
SOURCES	= simple.cpp

INCLUDEPATH += . tmp
MOC_DIR	= tmp
OBJECTS_DIR = tmp
TARGET	= "simple dll"
DESTDIR	= "dest dir"
