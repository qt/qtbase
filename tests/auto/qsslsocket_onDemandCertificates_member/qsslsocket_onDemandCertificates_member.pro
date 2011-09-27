load(qttest_p4)

SOURCES += tst_qsslsocket_onDemandCertificates_member.cpp
!wince*:win32:LIBS += -lws2_32
QT += core-private network-private
QT -= gui

TARGET = tst_qsslsocket_onDemandCertificates_member

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}

wince* {
    DEFINES += SRCDIR=\\\"./\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

requires(contains(QT_CONFIG,private_tests))
