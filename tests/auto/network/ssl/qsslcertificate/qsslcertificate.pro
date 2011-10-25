CONFIG += testcase

SOURCES += tst_qsslcertificate.cpp
!wince*:win32:LIBS += -lws2_32
QT = core network testlib

TARGET = tst_qsslcertificate

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}

wince* {
    certFiles.files = certificates more-certificates
    certFiles.path    = .
    DEPLOYMENT += certFiles
    DEFINES += SRCDIR=\\\".\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
