SOURCES = main.cpp
CONFIG -= qt
CONFIG += console
DESTDIR = ./

mac {
  CONFIG -= app_bundle
}

# no install rule for application used by test
INSTALLS =

