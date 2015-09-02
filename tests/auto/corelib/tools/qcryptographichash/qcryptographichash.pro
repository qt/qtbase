CONFIG += testcase
TARGET = tst_qcryptographichash
QT = core testlib
SOURCES = tst_qcryptographichash.cpp

TESTDATA += data/*

android: !android-no-sdk {
    RESOURCES += \
        testdata.qrc
}
