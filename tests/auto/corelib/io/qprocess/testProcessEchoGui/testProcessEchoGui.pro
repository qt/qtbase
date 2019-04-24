win32 {
   SOURCES = main_win.cpp
   QMAKE_USE += user32
}

CONFIG -= qt app_bundle
DESTDIR = ./
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
