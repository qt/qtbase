SOURCES = main.cpp
CONFIG -= qt
CONFIG += cmdline
DESTDIR = ./
OBJECTS_DIR = $${OBJECTS_DIR}-twospaces

TARGET = "two space s"
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
QT = core
