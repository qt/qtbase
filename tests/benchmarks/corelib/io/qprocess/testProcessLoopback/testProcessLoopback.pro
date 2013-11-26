SOURCES = main.cpp
CONFIG -= qt app_bundle
CONFIG += console
winrt: QMAKE_LFLAGS += /ENTRY:mainCRTStartup
DESTDIR = ./
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
