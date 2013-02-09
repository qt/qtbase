SOURCES = main.cpp
CONFIG -= qt app_bundle
CONFIG += console
DESTDIR = ./
OBJECTS_DIR = $${OBJECTS_DIR}-twospaces

TARGET = "two space s"
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
QT = core
