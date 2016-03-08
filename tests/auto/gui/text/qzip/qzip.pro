CONFIG += testcase
TARGET = tst_qzip
QT += gui-private testlib
SOURCES += tst_qzip.cpp

android {
    RESOURCES += \
        testdata.qrc
}
