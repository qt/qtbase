SOURCES = iconv.cpp
CONFIG -= qt dylib
mac|mingw|qnx|haiku:LIBS += -liconv
