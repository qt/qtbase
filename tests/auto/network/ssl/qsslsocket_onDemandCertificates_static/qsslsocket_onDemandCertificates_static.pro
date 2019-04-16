CONFIG += testcase

SOURCES += tst_qsslsocket_onDemandCertificates_static.cpp
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

#DOCKERTODO Linux, docker is disabled on macOS and Windows.
linux {
    QT_TEST_SERVER_LIST = squid danted
    include($$dirname(_QMAKE_CONF_)/tests/auto/testserver.pri)
}
