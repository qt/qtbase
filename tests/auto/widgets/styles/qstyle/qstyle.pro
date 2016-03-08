CONFIG += testcase
TARGET = tst_qstyle
QT += widgets testlib
SOURCES  += tst_qstyle.cpp

android {
    RESOURCES += \
        testdata.qrc
}
