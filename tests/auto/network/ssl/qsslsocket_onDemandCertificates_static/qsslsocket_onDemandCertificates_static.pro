CONFIG += testcase

SOURCES += tst_qsslsocket_onDemandCertificates_static.cpp
win32:LIBS += -lws2_32
QT = core core-private network-private testlib

TARGET = tst_qsslsocket_onDemandCertificates_static

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}

DEFINES += SRCDIR=\\\"$$PWD/\\\"

requires(qtConfig(private_tests))
