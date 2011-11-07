CONFIG += testcase parallel_test
TARGET = tst_qtextboundaryfinder
QT = core testlib
SOURCES = tst_qtextboundaryfinder.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"

wince* {
   addFiles.files = data
   addFiles.path = .
   DEPLOYMENT += addFiles
}
