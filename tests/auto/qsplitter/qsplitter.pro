load(qttest_p4)
QT += widgets
SOURCES  += tst_qsplitter.cpp

wince* {
   addFiles.files = extradata.txt setSizes3.dat
   addFiles.path = .
   DEPLOYMENT += addFiles
   DEFINES += SRCDIR=\\\"./\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
