load(qttest_p4)
SOURCES += tst_qtextbrowser.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += widgets

wince* {
   addFiles.files = *.html
   addFiles.path = .
   addDir.files = subdir/*
   addDir.path = subdir
   DEPLOYMENT += addFiles addDir
}

CONFIG+=insignificant_test
