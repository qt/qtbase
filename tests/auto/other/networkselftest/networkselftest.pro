CONFIG += testcase
TARGET = tst_networkselftest

SOURCES += tst_networkselftest.cpp
QT = core core-private network testlib

CONFIG += unsupported/testserver
QT_TEST_SERVER_LIST = cyrus vsftpd apache2 ftp-proxy danted squid echo
