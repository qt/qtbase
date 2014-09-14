TEMPLATE = app
TARGET = ../main
DEPENDPATH += .

# Input
SOURCES += main.cpp
QT -= network core gui

LIBS +=  -lppapi -lnacl_io -L../ -llib
