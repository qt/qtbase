SOURCES = main.cpp
CONFIG -= qt
CONFIG += console
DESTDIR = ./

mac {
  CONFIG -= app_bundle
}
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
