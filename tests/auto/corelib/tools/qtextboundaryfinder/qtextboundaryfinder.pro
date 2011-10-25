CONFIG += testcase
TARGET = tst_qtextboundaryfinder
QT = core testlib
HEADERS += 
SOURCES += tst_qtextboundaryfinder.cpp 
DEFINES += SRCDIR=\\\"$$PWD\\\"

wince* {
   addFiles.files = data
   addFiles.path = .
   DEPLOYMENT += addFiles
}
CONFIG += parallel_test
