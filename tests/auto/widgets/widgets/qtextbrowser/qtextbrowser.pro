CONFIG += testcase
TARGET = tst_qtextbrowser
SOURCES += tst_qtextbrowser.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += widgets testlib

wince* {
   addFiles.files = *.html
   addFiles.path = .
   addDir.files = subdir/*
   addDir.path = subdir
   DEPLOYMENT += addFiles addDir
}
