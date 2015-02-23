TEMPLATE		= app
HEADERS		= test_file.h
SOURCES		= test_file.cpp \
		  	main.cpp
TARGET	= "simple app"
DESTDIR	= "dest dir"

INCLUDEPATH += ../simple_dll
LIBS += -L"../simple_dll/dest dir" -l"simple dll"
