SOURCES = iconv.cpp
CONFIG -= qt dylib
mac|win32-g++*|qnx:LIBS += -liconv
