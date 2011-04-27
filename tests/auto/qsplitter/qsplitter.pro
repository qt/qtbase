load(qttest_p4)
SOURCES  += tst_qsplitter.cpp


contains(QT_CONFIG, qt3support): QT += qt3support

wince*|symbian: {
   addFiles.files = extradata.txt setSizes3.dat
   addFiles.path = .
   DEPLOYMENT += addFiles
   !symbian:DEFINES += SRCDIR=\\\"./\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
