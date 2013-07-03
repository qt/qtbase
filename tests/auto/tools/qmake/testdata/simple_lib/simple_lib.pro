TEMPLATE	= lib
CONFIG += release staticlib
CONFIG -= dll

HEADERS	= simple.h
SOURCES	= simple.cpp

VERSION     = 1.0.0
INCLUDEPATH += . tmp
MOC_DIR	= tmp
OBJECTS_DIR = tmp
TARGET	= simple_lib
DESTDIR	= ./
