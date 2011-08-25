load(qttest_p4)
SOURCES         += ../tst_qlibrary.cpp
TARGET  = ../tst_qlibrary
QT = core

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_qlibrary
} else {
    TARGET = ../../release/tst_qlibrary
  }
}

wince*: {
   addFiles.files = ../*.dll ../*.dl2 ../mylib_noextension
   addFiles.path = .
   DEPLOYMENT += addFiles
   DEFINES += SRCDIR=\\\"\\\"
}else:symbian {
   binDep.files = \
        mylib.dll \
        system.trolltech.test.mylib.dll
   binDep.path = /sys/bin
#mylib.dl2 nonstandard binary deployment will cause warning in emulator,
#but it can be safely ignored.
   custBinDep.files = mylib.dl2
   custBinDep.path = /sys/bin

   DEPLOYMENT += binDep custBinDep
} else {
   DEFINES += SRCDIR=\\\"$$PWD/../\\\"
}

