SOURCES = icu.cpp
CONFIG -= qt dylib
unix:LIBS += -licuuc -licui18n
win32:LIBS += -licuin
