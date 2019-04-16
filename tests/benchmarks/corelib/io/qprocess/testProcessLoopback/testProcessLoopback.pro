SOURCES = main.cpp
CONFIG -= qt
CONFIG += cmdline
winrt: QMAKE_LFLAGS += /ENTRY:mainCRTStartup
DESTDIR = ./
