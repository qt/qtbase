load(qttest_p4)
SOURCES  += tst_qbytearray.cpp


QT = core core-private

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
