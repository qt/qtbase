CONFIG += testcase
testcase.timeout = 300 # this test is slow

SOURCES += tst_qsslsocket_onDemandCertificates_member.cpp
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

# DOCKERTODO: linux, docker is disabled on macOS/Windows.
linux {
    QT_TEST_SERVER_LIST = squid danted
    include($$dirname(_QMAKE_CONF_)/tests/auto/testserver.pri)
}
