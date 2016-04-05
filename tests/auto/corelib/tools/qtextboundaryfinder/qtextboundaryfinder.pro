CONFIG += testcase
TARGET = tst_qtextboundaryfinder
QT = core testlib
SOURCES = tst_qtextboundaryfinder.cpp

TESTDATA += data

android {
    RESOURCES += \
        testdata.qrc
}
