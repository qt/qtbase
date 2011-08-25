TEMPLATE      = lib
CONFIG       += plugin
HEADERS       = almostplugin.h
SOURCES       = almostplugin.cpp
TARGET        = almostplugin
DESTDIR       = ../bin
*-g++*:QMAKE_LFLAGS -= -Wl,--no-undefined
