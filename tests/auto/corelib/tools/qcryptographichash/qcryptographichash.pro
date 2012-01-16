CONFIG += testcase parallel_test
TARGET = tst_qcryptographichash
QT = core testlib
SOURCES = tst_qcryptographichash.cpp


wince* {
   addFiles.files = data/*
   addFiles.path = data/
   DEPLOYMENT += addFiles

   DEFINES += SRCDIR=\\\".\\\"
}
else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}