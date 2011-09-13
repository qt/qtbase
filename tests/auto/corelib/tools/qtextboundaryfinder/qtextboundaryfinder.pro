load(qttest_p4)
QT = core
HEADERS += 
SOURCES += tst_qtextboundaryfinder.cpp 
!symbian:*:DEFINES += SRCDIR=\\\"$$PWD\\\"

wince*|symbian:{
   addFiles.files = data
   addFiles.path = .
   DEPLOYMENT += addFiles
}
CONFIG += parallel_test
