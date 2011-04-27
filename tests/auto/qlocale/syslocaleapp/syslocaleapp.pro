SOURCES += syslocaleapp.cpp
DESTDIR = ./

QT = core

# no install rule for application used by test
INSTALLS =
CONFIG -= app_bundle
