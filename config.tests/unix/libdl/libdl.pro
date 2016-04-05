SOURCES = libdl.cpp
CONFIG -= qt dylib
!qnx: LIBS += -ldl
