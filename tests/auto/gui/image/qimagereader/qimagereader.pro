CONFIG += testcase
TARGET = tst_qimagereader
SOURCES += tst_qimagereader.cpp
MOC_DIR=tmp
QT += core-private gui-private network testlib
RESOURCES += qimagereader.qrc

android:!android-embedded {
    RESOURCES += android_testdata.qrc
}

TESTDATA += images/* baseline/*
