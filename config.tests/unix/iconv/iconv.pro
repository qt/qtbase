SOURCES = iconv.cpp
CONFIG -= qt dylib
mac|mingw|openbsd|qnx|haiku:LIBS += -liconv
