CONFIG += testcase
CONFIG += parallel_test

SOURCES += tst_qsslkey.cpp
!wince*:win32:LIBS += -lws2_32
QT = core network testlib
contains(QT_CONFIG, private_tests) {
    QT += core-private network-private
}

TARGET = tst_qsslkey

TESTDATA += keys/* rsa-without-passphrase.pem rsa-with-passphrase.pem
