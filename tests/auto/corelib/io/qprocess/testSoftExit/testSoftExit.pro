win32 {
   SOURCES = main_win.cpp
   !wince: LIBS += -luser32
}
unix {
   SOURCES = main_unix.cpp
}

CONFIG -= qt app_bundle
CONFIG += console
DESTDIR = ./
QT = core
