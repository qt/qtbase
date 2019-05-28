CONFIG += testcase
SOURCES  += ../tst_qtcpserver.cpp

win32: QMAKE_USE += ws2_32

TARGET = ../tst_qtcpserver

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_qtcpserver
} else {
    TARGET = ../../release/tst_qtcpserver
  }
}

QT = core network testlib

MOC_DIR=tmp

# Only on Linux until cyrus has been added to docker-compose-for-{windows,macOS}.yml and tested
linux {
    CONFIG += unsupported/testserver
    QT_TEST_SERVER_LIST = danted cyrus squid ftp-proxy
}
