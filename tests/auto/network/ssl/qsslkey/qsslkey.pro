load(qttest_p4)

SOURCES += tst_qsslkey.cpp
!wince*:win32:LIBS += -lws2_32
QT = core network

TARGET = tst_qsslkey

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}

wince*|symbian: {
   keyFiles.files = keys
   keyFiles.path    = .

   passphraseFiles.files = rsa-without-passphrase.pem rsa-with-passphrase.pem
   passphraseFiles.path    = .

   DEPLOYMENT += keyFiles passphraseFiles
}

wince*: {
   DEFINES += SRCDIR=\\\".\\\"
} else:!symbian {
   DEFINES+= SRCDIR=\\\"$$PWD\\\"
   TARGET.CAPABILITY = NetworkServices
}
