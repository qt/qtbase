CONFIG += testcase
TARGET = tst_qdom
SOURCES  += tst_qdom.cpp

QT = core xml testlib

wince* {
   wince*|qt_not_deployed {
       DEPLOYMENT_PLUGIN += qcncodecs qjpcodecs qkrcodecs qtwcodecs
   }
}
TESTDATA += testdata/* doubleNamespaces.xml umlaut.xml
