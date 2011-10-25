CONFIG += testcase
TARGET = tst_qdom
SOURCES  += tst_qdom.cpp

QT = core xml testlib
QT -= gui

wince* {
   addFiles.files = testdata doubleNamespaces.xml umlaut.xml
   addFiles.path = .
   DEPLOYMENT += addFiles

   wince*|qt_not_deployed {
       DEPLOYMENT_PLUGIN += qcncodecs qjpcodecs qkrcodecs qtwcodecs
   }
   DEFINES += SRCDIR=\\\"\\\"
}
else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
