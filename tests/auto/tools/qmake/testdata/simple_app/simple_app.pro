TEMPLATE		= app
CONFIG		+= qt warn_on
HEADERS		= test_file.h
SOURCES		= test_file.cpp \
		  	main.cpp
RESOURCES = test.qrc
TARGET		= simple_app
DESTDIR		= ./

infile($(QTDIR)/.qmake.cache, CONFIG, debug):CONFIG += debug
infile($(QTDIR)/.qmake.cache, CONFIG, release):CONFIG += release


