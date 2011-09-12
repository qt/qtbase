TEMPLATE		= app
CONFIG		+= qt warn_on
SOURCES		= main.cpp
TARGET		= "one space"
DESTDIR		= ./

infile($(QTDIR)/.qmake.cache, CONFIG, debug):CONFIG += debug
infile($(QTDIR)/.qmake.cache, CONFIG, release):CONFIG += release


