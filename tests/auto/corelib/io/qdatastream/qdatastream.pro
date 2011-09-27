load(qttest_p4)
SOURCES += tst_qdatastream.cpp
QT += gui widgets
wince*: {
   addFiles.files = datastream.q42
   addFiles.path = .
   DEPLOYMENT += addFiles
   DEFINES += SRCDIR=\\\"\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

