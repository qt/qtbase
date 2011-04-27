load(qttest_p4)
SOURCES += tst_qtextbrowser.cpp
!symbian:DEFINES += SRCDIR=\\\"$$PWD\\\"

contains(QT_CONFIG, qt3support): QT += qt3support


wince*|symbian: {
   addFiles.files = *.html
   addFiles.path = .
   addDir.files = subdir/*
   addDir.path = subdir
   DEPLOYMENT += addFiles addDir
}



