win32 {
   SOURCES = main_win.cpp
   !win32-borland:LIBS += -lUser32
}

CONFIG -= qt
DESTDIR = ./

# no install rule for application used by test
INSTALLS =



