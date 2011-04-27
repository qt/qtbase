load(qttest_p4)

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

wince*|symbian {
   addFiles.files = ../*.txt
   addFiles.path = .
   DEPLOYMENT += addFiles
   wince*|qt_not_deployed {
       DEPLOYMENT_PLUGIN += qcncodecs qjpcodecs qkrcodecs qtwcodecs
   }
}

wince*: {
   DEFINES += SRCDIR=\\\"\\\"
}else:symbian {
    # Symbian can't define SRCDIR meaningfully here
    LIBS += -lcharconv -lconvnames -lgb2312_shared -ljisx0201 -ljisx0208 -lefsrv
} else {
   DEFINES += SRCDIR=\\\"$$PWD/../\\\"
}
