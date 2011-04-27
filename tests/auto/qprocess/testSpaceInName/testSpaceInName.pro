SOURCES = main.cpp
CONFIG -= qt
CONFIG += console
DESTDIR = "../test Space In Name"

mac {
  CONFIG -= app_bundle
}

# no install rule for application used by test
INSTALLS =


