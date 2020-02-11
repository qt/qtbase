CONFIG += testcase
TARGET = tst_qftp
SOURCES  += tst_qftp.cpp
QT_FOR_CONFIG += network

requires(qtConfig(ftp))
requires(qtConfig(private_tests))
QT = core network network-private testlib

CONFIG += unsupported/testserver
QT_TEST_SERVER_LIST = vsftpd ftp-proxy squid danted
