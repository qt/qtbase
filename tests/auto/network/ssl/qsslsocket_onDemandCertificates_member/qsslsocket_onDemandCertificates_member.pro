CONFIG += testcase
testcase.timeout = 300 # this test is slow

SOURCES += tst_qsslsocket_onDemandCertificates_member.cpp
win32:LIBS += -lws2_32
QT = core core-private network-private testlib

TARGET = tst_qsslsocket_onDemandCertificates_member

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}

DEFINES += SRCDIR=\\\"$$PWD/\\\"

requires(qtConfig(private_tests))
