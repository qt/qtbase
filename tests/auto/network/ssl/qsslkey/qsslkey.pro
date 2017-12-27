CONFIG += testcase

SOURCES += tst_qsslkey.cpp
win32:LIBS += -lws2_32
QT = core network testlib
qtConfig(private_tests) {
    QT += core-private network-private
}

TARGET = tst_qsslkey

TESTDATA += keys/* \
            rsa-*.pem
