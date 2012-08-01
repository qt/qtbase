CONFIG += testcase
CONFIG += parallel_test
testcase.timeout = 300 # this test is slow

SOURCES += tst_qsslsocket_onDemandCertificates_member.cpp
!wince*:win32:LIBS += -lws2_32
QT += core-private network-private testlib
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
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
