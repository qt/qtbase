load(qttest_p4)
SOURCES  += tst_qdom.cpp

QT = core xml
QT -= gui

wince*|symbian: {
   addFiles.files = testdata doubleNamespaces.xml umlaut.xml
   addFiles.path = .
   DEPLOYMENT += addFiles

   wince*|qt_not_deployed {
       DEPLOYMENT_PLUGIN += qcncodecs qjpcodecs qkrcodecs qtwcodecs
   }
   !symbian:DEFINES += SRCDIR=\\\"\\\"
}
else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

symbian: TARGET.EPOCHEAPSIZE="0x100000 0x1000000"
