CONFIG += testcase
TARGET = tst_qbytearray
SOURCES  += tst_qbytearray.cpp


QT = core core-private testlib

wince* {
   addFile.files = rfc3252.txt
   addFile.path = .
   DEPLOYMENT += addFile
}

wince* {
  DEFINES += SRCDIR=\\\"./\\\"
} else {
  DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

CONFIG += parallel_test
