CONFIG += testcase
TARGET = tst_qlayout

QT += widgets widgets-private testlib

SOURCES += tst_qlayout.cpp
TESTDATA += baseline/*

android {
    RESOURCES += \
        testdata.qrc
}
