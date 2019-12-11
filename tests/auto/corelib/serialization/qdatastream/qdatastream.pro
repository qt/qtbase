CONFIG += testcase
TARGET = tst_qdatastream
QT += testlib
SOURCES = tst_qdatastream.cpp

DEFINES -= QT_NO_LINKED_LIST

TESTDATA += datastream.q42

android:!android-embedded {
    RESOURCES += \
        testdata.qrc
}
