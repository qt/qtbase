SOURCES = odbc.cpp
CONFIG -= qt dylib
win32-g++*:LIBS += -lodbc32
else:LIBS += -lodbc
