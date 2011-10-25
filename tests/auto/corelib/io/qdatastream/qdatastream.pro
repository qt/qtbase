CONFIG += testcase
TARGET = tst_qdatastream
SOURCES += tst_qdatastream.cpp
QT += gui widgets testlib
wince*: {
   addFiles.files = datastream.q42
   addFiles.path = .
   DEPLOYMENT += addFiles
   DEFINES += SRCDIR=\\\"\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

