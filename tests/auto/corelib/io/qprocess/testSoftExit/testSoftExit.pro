win32 {
   SOURCES = main_win.cpp
   LIBS += -luser32
}
unix {
   SOURCES = main_unix.cpp
}

CONFIG -= qt
CONFIG += cmdline
DESTDIR = ./
QT = core
