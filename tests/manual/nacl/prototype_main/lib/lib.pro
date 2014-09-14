TEMPLATE = lib
TARGET = ../lib
DEPENDPATH += .

# Input
SOURCES += lib.cpp
QT -= network core gui

LIBS +=  -lppapi -lnacl_io