SOURCES = main.cpp
CONFIG -= qt
CONFIG += console
DESTDIR = ./

mac {
  CONFIG -= app_bundle
}
