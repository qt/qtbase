CONFIG += benchmark
CONFIG -= qt
CONFIG += cmdline
winrt: QMAKE_LFLAGS += /ENTRY:mainCRTStartup

SOURCES = main.cpp
DESTDIR = ./
