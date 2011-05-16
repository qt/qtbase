load(qttest_p4)

SOURCES += tst_qsslsocket.cpp
!wince*:win32:LIBS += -lws2_32
QT += core-private network-private
QT -= gui

TARGET = tst_qsslsocket

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}

wince* {
    DEFINES += SRCDIR=\\\"./\\\"

    certFiles.files = certs ssl.tar.gz
    certFiles.path    = .
    DEPLOYMENT += certFiles
} else:symbian {
    DEFINES += QSSLSOCKET_CERTUNTRUSTED_WORKAROUND
    TARGET.EPOCHEAPSIZE="0x100 0x3000000"
    TARGET.CAPABILITY=NetworkServices ReadUserData

    certFiles.files = certs ssl.tar.gz
    certFiles.path    = .
    DEPLOYMENT += certFiles
    INCLUDEPATH *= $$MW_LAYER_SYSTEMINCLUDE  # Needed for e32svr.h in S^3 envs
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

requires(contains(QT_CONFIG,private_tests))
