SOURCES = main.cpp
CONFIG -= qt app_bundle
CONFIG += console
winrt: QMAKE_LFLAGS += /ENTRY:mainCRTStartup
DESTDIR = ./
