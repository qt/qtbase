CONFIG += testcase

SOURCES += tst_qsslkey.cpp
QT = core network testlib
qtConfig(private_tests) {
    QT += core-private network-private
}

TARGET = tst_qsslkey

TESTDATA += keys/* \
            rsa-*.pem
