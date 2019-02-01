SOURCES = main.cpp
CONFIG -= qt
CONFIG += cmdline

win32:!mingw:!equals(TEMPLATE_PREFIX, "vc"):QMAKE_CXXFLAGS += /GS-
DESTDIR = ./
