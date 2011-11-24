win32 {
   SOURCES = main_win.cpp
   !win32-borland:LIBS += -lUser32
}

CONFIG -= qt
DESTDIR = ./

# This app is testdata for tst_qprocess
target.path = $$[QT_INSTALL_TESTS]/tst_qprocess/$$TARGET
INSTALLS += target
