CONFIG += testcase
QT += testlib

SOURCES  += ../tst_qtextcodec.cpp

!wince*: {
TARGET = ../tst_qtextcodec

win32: {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_qtextcodec
} else {
    TARGET = ../../release/tst_qtextcodec
  }
}
} else {
   TARGET = tst_qtextcodec
}

wince* {
   addFiles.files = ../*.txt
   addFiles.path = .
   DEPLOYMENT += addFiles
   wince*|qt_not_deployed {
       DEPLOYMENT_PLUGIN += qcncodecs qjpcodecs qkrcodecs qtwcodecs
   }
}

wince*: {
   DEFINES += SRCDIR=\\\"\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD/../\\\"
}
