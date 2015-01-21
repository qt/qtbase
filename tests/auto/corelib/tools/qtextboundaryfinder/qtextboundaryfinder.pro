CONFIG += testcase parallel_test
TARGET = tst_qtextboundaryfinder
QT = core testlib
SOURCES = tst_qtextboundaryfinder.cpp

TESTDATA += data
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

android: !android-no-sdk {
    RESOURCES += \
        testdata.qrc
}
