CONFIG += testcase
TARGET = tst_qdatastream
QT += testlib
SOURCES = tst_qdatastream.cpp

TESTDATA += datastream.q42
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

android: !android-no-sdk {
    RESOURCES += \
        testdata.qrc
}
