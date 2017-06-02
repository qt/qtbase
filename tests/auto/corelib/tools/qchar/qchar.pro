CONFIG += testcase
TARGET = tst_qchar
QT = core-private testlib
SOURCES = tst_qchar.cpp

TESTDATA += data/NormalizationTest.txt

android:!android-embedded {
    RESOURCES += \
        testdata.qrc
}
