TEMPLATE      = lib
CONFIG       += plugin
HEADERS       = theplugin.h
SOURCES       = theplugin.cpp
TARGET        = $$qtLibraryTarget(theplugin)
DESTDIR       = ../bin

symbian: {
    TARGET.EPOCALLOWDLLDATA=1
    TARGET.CAPABILITY=ALL -TCB
}
