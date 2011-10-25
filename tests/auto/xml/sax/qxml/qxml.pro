CONFIG += testcase
TARGET = tst_qxml

SOURCES += tst_qxml.cpp
QT = core xml testlib

wince* {
   addFiles.files = 0x010D.xml
   addFiles.path = .
   DEPLOYMENT += addFiles
}
