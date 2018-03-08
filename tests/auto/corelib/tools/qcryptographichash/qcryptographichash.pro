CONFIG += testcase
TARGET = tst_qcryptographichash
QT = core testlib
SOURCES = tst_qcryptographichash.cpp

TESTDATA += data/*

android:!android-embedded {
    RESOURCES += \
        testdata.qrc
}
