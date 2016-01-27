CONFIG += testcase

SOURCES += tst_qsslsocket.cpp
win32:!wince: LIBS += -lws2_32
QT = core core-private network-private testlib

TARGET = tst_qsslsocket

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}

# OpenSSL support
contains(QT_CONFIG, openssl) | contains(QT_CONFIG, openssl-linked) {
    # Add optional SSL libs
    LIBS += $$OPENSSL_LIBS
}

wince* {
    DEFINES += SRCDIR=\\\"./\\\"

    certFiles.files = certs ssl.tar.gz
    certFiles.path    = .
    DEPLOYMENT += certFiles
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

requires(contains(QT_CONFIG,private_tests))
