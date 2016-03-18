CONFIG += testcase
TARGET = tst_qcryptographichash
QT = core testlib
SOURCES = tst_qcryptographichash.cpp

TESTDATA += data/*

android {
    RESOURCES += \
        testdata.qrc
}
