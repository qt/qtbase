CONFIG += testcase
TARGET = tst_qcomplextext
QT += testlib
QT += core-private gui-private
SOURCES  += tst_qcomplextext.cpp

TESTDATA += data

android {
    RESOURCES += \
        android_testdata.qrc
}

builtin_testdata: DEFINES += BUILTIN_TESTDATA
